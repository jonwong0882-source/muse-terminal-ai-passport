#!/usr/bin/env python3
"""Local speech + TLS bridge with a dedicated Python-controlled Muse browser."""
import argparse,asyncio,contextlib,json,os,secrets,ssl,subprocess,threading,time
from pathlib import Path
from aiohttp import web
from protocol import *
from muse_web import MuseWeb
DEFAULT_STATE=Path.home()/'Library/Application Support/MuseTerminal'

class Speech:
    def __init__(self,model='base'):
        self.model_name=model;self.model=None;self.lock=threading.Lock()
    def load(self):
        from faster_whisper import WhisperModel
        self.model=WhisperModel(self.model_name,device='cpu',compute_type='int8',cpu_threads=4)
    def transcribe(self,pcm):
        import numpy as np
        with self.lock:
            segments,_=self.model.transcribe(np.frombuffer(pcm,dtype='<i2').astype(np.float32)/32768,language='zh',vad_filter=True,beam_size=3,condition_on_previous_text=False)
            return ''.join(s.text for s in segments).strip()


class Device:
    def __init__(self,reader,writer,muse,speech,token):
        self.reader=reader;self.writer=writer;self.muse=muse;self.speech=speech;self.token=token
        self.decoder=Decoder();self.auth=False;self.session=0;self.pcm=bytearray();self.phase='idle';self.transcript='';self.task=None;self.last_hello=time.monotonic()
    async def send(self,kind,data=b'',session=None):
        self.writer.write(encode(kind,self.session if session is None else session,data));await self.writer.drain()
    async def stop(self):
        if self.task:
            self.task.cancel()
            with contextlib.suppress(asyncio.CancelledError,Exception):await self.task
        self.task=None;self.pcm.clear();self.transcript='';self.phase='idle'
    async def process_audio(self,session,pcm):
        try:
            text=await asyncio.wait_for(asyncio.to_thread(self.speech.transcribe,pcm),80)
            if session!=self.session:return
            if not text:raise RuntimeError('未识别到清晰语音，请重新录音。')
            if len(text.encode('utf-8'))>1900:raise RuntimeError('语音过长，请缩短录音。')
            self.transcript=text;self.phase='review';await self.send(TRANSCRIPT,text.encode('utf-8'))
        except asyncio.CancelledError:raise
        except Exception:
            self.phase='error';await self.send(ERROR,'语音识别失败或超时，请重新录音。'.encode())
    async def answer(self,session,text):
        try:
            await self.send(STATUS,'正在向你的 Muse 会话发送指令。'.encode())
            reply=await self.muse.ask(text)
            if self.session!=session:return
            await self.send(REPLY,text_bytes(reply));self.phase='reply'
        except asyncio.CancelledError:raise
        except Exception as exc:
            self.phase='error'
            message=str(exc) if isinstance(exc,RuntimeError) else '请求超时或连接中断。请在 Muse 查看状态，不会自动重复发送。'
            await self.send(ERROR,text_bytes(message))
    async def frame(self,kind,session,data):
        if kind==HELLO:
            if not secrets.compare_digest(data,self.token.encode()):raise PermissionError('device pairing mismatch')
            self.auth=True;self.last_hello=time.monotonic();return
        if not self.auth:raise PermissionError('pair first')
        if kind==START:
            await self.stop();self.session=session;self.phase='recording';return
        if session!=self.session:return
        if kind==AUDIO and self.phase=='recording':
            if len(data)%2 or len(self.pcm)+len(data)>16000*2*31:
                await self.stop();await self.send(ERROR,'录音数据超限，请重试。'.encode());return
            self.pcm.extend(data)
        elif kind==END and self.phase=='recording':
            self.phase='transcribing';pcm=bytes(self.pcm);self.pcm.clear()
            self.task=asyncio.create_task(self.process_audio(session,pcm))
        elif kind==CONFIRM and self.phase=='review':
            self.phase='waiting';self.task=asyncio.create_task(self.answer(session,self.transcript))
        elif kind==CANCEL:await self.stop()
    async def heartbeat(self):
        while True:
            if time.monotonic()-self.last_hello>8:raise TimeoutError('device heartbeat missing')
            if self.auth:await self.send(PING,bytes([self.muse.online and self.speech.model is not None]),session=0)
            await asyncio.sleep(1)
    async def read(self):
        while chunk:=await self.reader.read(4096):
            for frame in self.decoder.feed(chunk):await self.frame(*frame)
    async def run(self):
        tasks=[asyncio.create_task(self.read()),asyncio.create_task(self.heartbeat())]
        try:
            done,_=await asyncio.wait(tasks,return_when=asyncio.FIRST_COMPLETED)
            for t in done:t.result()
        finally:
            for t in tasks:t.cancel()
            await asyncio.gather(*tasks,return_exceptions=True);await self.stop();self.writer.close()
            with contextlib.suppress(Exception):await self.writer.wait_closed()

def state_files(root):
    root.mkdir(parents=True,exist_ok=True);os.chmod(root,0o700)
    config=root/'pairing.json'
    if not config.exists():
        config.write_text(json.dumps({'device_token':secrets.token_urlsafe(32),'extension_token':secrets.token_urlsafe(32)}));os.chmod(config,0o600)
    settings=json.loads(config.read_text())
    key=root/'bridge-key.pem';cert=root/'bridge-cert.pem'
    if not key.exists() and not cert.exists():
        subprocess.run(['openssl','req','-x509','-newkey','rsa:2048','-nodes','-keyout',str(key),'-out',str(cert),'-days','3650','-subj','/CN=muse-bridge'],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL);os.chmod(key,0o600)
    if not key.exists() or not cert.exists():raise RuntimeError('Incomplete TLS identity; preserve existing files and repair manually.')
    return settings,key,cert

def create_app(muse,speech):
    @web.middleware
    async def local_only(request,handler):
        if request.host not in ('127.0.0.1:18765','localhost:18765'):
            raise web.HTTPForbidden()
        return await handler(request)
    app=web.Application(middlewares=[local_only])
    async def status(request):
        return web.json_response({
            'speech_ready':speech.model is not None,
            'muse_ready':muse.online,
            'muse_reason':muse.reason,
            'job_active':muse.lock.locked(),
            'browser_mode':'python',
        })
    async def home(request):
        return web.Response(
            text='<!doctype html><meta charset="utf-8"><title>Muse 终端桥接</title>'
                 '<style>body{background:#10151e;color:#e9f0f7;font:18px system-ui;'
                 'max-width:680px;margin:60px auto;padding:20px}a{color:#7de2cb}</style>'
                 '<h1>Muse 终端 / Mac 桥接</h1>'
                 '<p>本机语音识别 → Python 控制的 Muse 专用浏览器 → 无线终端</p>'
                 '<p>首次使用请在自动打开的专用 Chrome 窗口登录 Muse，然后进入已配置的终端会话。'
                 '保持输入框为空，审批操作仍由你在该窗口完成。</p>'
                 '<p><a href="/health">查看连接状态</a></p>',
            content_type='text/html',
            headers={'Cache-Control':'no-store',
                     'Content-Security-Policy':"default-src 'none'; style-src 'unsafe-inline'; frame-ancestors 'none'"})
    app.add_routes([web.get('/',home),web.get('/health',status)])
    return app

async def serve(args):
    settings,key,cert=state_files(args.state);muse=MuseWeb(args.muse_url,args.state/"browser-profile");speech=Speech(args.model)
    async def load():
        try:await asyncio.to_thread(speech.load);print('Local speech model ready',flush=True)
        except Exception:print('Speech model failed to load; check model download/network',flush=True)
    loader=asyncio.create_task(load())
    browser_task=asyncio.create_task(muse.run())
    app=create_app(muse,speech);runner=web.AppRunner(app,access_log=None);await runner.setup();await web.TCPSite(runner,'127.0.0.1',18765).start()
    context=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);context.minimum_version=ssl.TLSVersion.TLSv1_2;context.load_cert_chain(cert,key)
    active=set()
    async def accept(reader,writer):
        # Keep at most one authenticated terminal; unauthenticated probes get 8 s.
        device=Device(reader,writer,muse,speech,settings['device_token'])
        if active:writer.close();return
        active.add(device)
        try:await device.run()
        except Exception:print('Device disconnected; no command is automatically retried',flush=True)
        finally:active.discard(device)
    server=await asyncio.start_server(accept,args.bind,18766,ssl=context,ssl_handshake_timeout=8)
    print('Bridge running. Setup: http://127.0.0.1:18765 (TLS device port 18766)',flush=True)
    try:
        async with server:await server.serve_forever()
    finally:
        loader.cancel();browser_task.cancel()
        await asyncio.gather(loader,browser_task,return_exceptions=True)
        await runner.cleanup()

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--state',type=Path,default=DEFAULT_STATE);parser.add_argument('--model',default='base');parser.add_argument('--muse-url',default=os.environ.get('MUSE_URL'),required=not os.environ.get('MUSE_URL'));parser.add_argument('--bind',default='0.0.0.0');args=parser.parse_args()
    try:asyncio.run(serve(args))
    except KeyboardInterrupt:pass

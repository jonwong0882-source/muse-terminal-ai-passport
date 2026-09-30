"""Real local TLS and HTTP transport, simulated board/browser, deterministic speech."""
import asyncio,contextlib,ssl,sys,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'companion'))
from bridge import Device,create_app,state_files
from protocol import *
from aiohttp.test_utils import TestClient,TestServer
class SpeechFixture:
    model=True
    def transcribe(self,pcm):return '连接测试' if pcm else ''
class MuseFixture:
    online=True
    reason="ready"
    def __init__(self):self.commands=[];self.lock=asyncio.Lock()
    async def ask(self,text):self.commands.append(text);return "终端连接成功"
class Integration(unittest.IsolatedAsyncioTestCase):
    async def test_tls_record_review_confirm_text_reply(self):
        with tempfile.TemporaryDirectory() as d:
            settings,key,cert=state_files(Path(d));muse=MuseFixture();speech=SpeechFixture()
            context=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);context.load_cert_chain(cert,key)
            devices=[]
            async def accept(r,w):
                device=Device(r,w,muse,speech,settings['device_token']);devices.append(asyncio.current_task())
                with contextlib.suppress(Exception):await device.run()
            server=await asyncio.start_server(accept,'127.0.0.1',0,ssl=context)
            port=server.sockets[0].getsockname()[1]
            client=ssl.create_default_context(cafile=str(cert))
            r,w=await asyncio.open_connection('127.0.0.1',port,ssl=client,server_hostname='muse-bridge')
            dec=Decoder();pending=[]
            async def receive(kind):
                deadline=asyncio.get_running_loop().time()+5
                while asyncio.get_running_loop().time()<deadline:
                    for frame in list(pending):
                        if frame[0]==kind:pending.remove(frame);return frame
                    chunk=await asyncio.wait_for(r.read(4096),3)
                    if not chunk:raise AssertionError('unexpected disconnect')
                    pending.extend(dec.feed(chunk))
                raise AssertionError('missing frame')
            async def send(k,data=b''):w.write(encode(k,77,data));await w.drain()
            try:
                await send(HELLO,settings['device_token'].encode());await send(START);await send(AUDIO,b'\0\0'*100);await send(END)
                self.assertEqual((await receive(TRANSCRIPT))[2].decode(),'连接测试')
                await send(CONFIRM);await receive(STATUS)
                self.assertEqual((await receive(REPLY))[2].decode(),'终端连接成功')
                self.assertEqual(muse.commands,['连接测试'])
                self.assertFalse(any(frame[0] in (PLAY,DONE) for frame in pending))
                with self.assertRaises(asyncio.TimeoutError):await asyncio.wait_for(r.read(4096),.15)
            finally:
                w.close();await w.wait_closed();server.close();await server.wait_closed();await asyncio.gather(*devices,return_exceptions=True)
    async def test_http_local_health_and_no_extension_route(self):
        muse=MuseFixture();app=create_app(muse,SpeechFixture())
        async with TestClient(TestServer(app)) as c:
            resp=await c.get('/health');self.assertEqual(resp.status,403)
            h={'Host':'127.0.0.1:18765'}
            resp=await c.get('/health',headers=h)
            self.assertEqual(resp.status,200)
            data=await resp.json()
            self.assertEqual(data['browser_mode'],'python')
            self.assertTrue(data['muse_ready'])
            resp=await c.post('/extension/poll',json={},headers=h)
            self.assertEqual(resp.status,404)
if __name__=='__main__':unittest.main()

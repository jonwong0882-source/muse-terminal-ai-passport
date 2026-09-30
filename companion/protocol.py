"""Bounded framing shared with the ESP32 implementation; no network side effects."""
import struct,zlib
HELLO,PING,START,AUDIO,END,TRANSCRIPT,CONFIRM,CANCEL,STATUS,REPLY,PLAY,ACK,DONE,ERROR,CONFIG,CONFIG_ACK=range(1,17)
MAX_PAYLOAD=2048

def encode(kind,session,data=b''):
    if len(data)>MAX_PAYLOAD: raise ValueError('oversized frame')
    head=struct.pack('<BBHI',kind,0,len(data),session)
    return b'MUS1'+head+struct.pack('<I',zlib.crc32(head+data))+data

class Decoder:
    def __init__(self): self.buffer=bytearray()
    def feed(self,data):
        self.buffer.extend(data)
        result=[]
        while True:
            start=self.buffer.find(b'MUS1')
            if start<0:
                self.buffer[:]=self.buffer[-3:];break
            del self.buffer[:start]
            if len(self.buffer)<16:break
            kind,reserved,n,session,crc=struct.unpack_from('<BBHII',self.buffer,4)
            if reserved or n>MAX_PAYLOAD:
                del self.buffer[:16];continue
            if len(self.buffer)<16+n:break
            payload=bytes(self.buffer[16:16+n]);head=bytes(self.buffer[4:12]);del self.buffer[:16+n]
            if zlib.crc32(head+payload)==crc:result.append((kind,session,payload))
        return result

def text_bytes(text,limit=2000):
    raw=text.encode('utf-8')
    if len(raw)<=limit:return raw
    suffix='\n全文请在 Muse 查看'.encode()
    return raw[:limit-len(suffix)].decode('utf-8','ignore').encode()+suffix

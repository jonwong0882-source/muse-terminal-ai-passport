#!/usr/bin/env python3
"""Provision an already-flashed Muse terminal over an explicitly selected USB port."""
import argparse,getpass,json,time
from pathlib import Path
import serial
from bridge import DEFAULT_STATE,state_files
from protocol import CONFIG,CONFIG_ACK,Decoder,encode
p=argparse.ArgumentParser();p.add_argument('--port',required=True);p.add_argument('--host',required=True,help='Mac LAN IPv4 address; reserve it in the router');p.add_argument('--state',type=Path,default=DEFAULT_STATE);args=p.parse_args()
settings,_,cert=state_files(args.state)
ssid=input('2.4 GHz Wi-Fi SSID: ');password=getpass.getpass('Wi-Fi password: ')
profile={'ssid':ssid,'password':password,'host':args.host,'port':18766,'token':settings['device_token'],'ca':cert.read_text(),'epoch':int(time.time())}
raw=json.dumps(profile,separators=(',',':'),ensure_ascii=False).encode()
if not 1<=len(ssid.encode())<=32 or len(password.encode())>63 or len(raw)>2048:raise SystemExit('Configuration too long')
# Setting modem control before opening avoids resetting the ESP32.
s=serial.Serial();s.port=args.port;s.baudrate=115200;s.timeout=.2;s.dtr=False;s.rts=False;s.open()
try:
    s.write(encode(CONFIG,1,raw));s.flush();decoder=Decoder();deadline=time.monotonic()+8
    while time.monotonic()<deadline:
        for kind,session,data in decoder.feed(s.read(4096)):
            if kind==CONFIG_ACK and session==1:
                if data!=b'OK':raise SystemExit('Device rejected configuration')
                print('Configuration saved; terminal is restarting. USB may now be disconnected.');raise SystemExit(0)
    raise SystemExit('No configuration acknowledgement; verify the Muse firmware and selected port')
finally:s.close()

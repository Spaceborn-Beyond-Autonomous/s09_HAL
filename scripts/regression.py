#!/usr/bin/env python3
"""Run the host regression and exercise the peripheral simulator bridge."""
from __future__ import annotations
import json, socket, subprocess, sys, threading, time
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]

def bridge_check():
    sys.path.insert(0,str(ROOT/'peripheral-sim'))
    from peripheral_sim import serve_socket
    port=18767
    threading.Thread(target=serve_socket,kwargs={'port':port},daemon=True).start()
    time.sleep(0.1)
    def call(req):
        with socket.create_connection(('127.0.0.1',port),timeout=2) as s:
            s.sendall((json.dumps(req)+'\n').encode())
            return json.loads(s.makefile('rb').readline())
    assert call({'op':'spi','cs':'imu','reg':0x75|0x80,'len':1})['data']==[0x47]
    assert call({'op':'spi','cs':'baro','reg':0x00|0x80,'len':1})['data']==[0x50]
    assert call({'op':'i2c','addr':0x1e,'reg':0x0a,'len':3})['data']==[ord('H'),ord('4'),ord('3')]
    return True

subprocess.run(['cmake','-S','.','-B','build','-DCMAKE_BUILD_TYPE=Debug'],cwd=ROOT,check=True)
subprocess.run(['cmake','--build','build'],cwd=ROOT,check=True)
subprocess.run(['ctest','--test-dir','build','--output-on-failure'],cwd=ROOT,check=True)
subprocess.run([sys.executable,'-m','pytest','-q','peripheral-sim/tests'],cwd=ROOT,check=True)
assert bridge_check()
print('PASS: complete host regression + peripheral socket bridge')

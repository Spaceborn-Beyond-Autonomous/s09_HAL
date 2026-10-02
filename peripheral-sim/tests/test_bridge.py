import json
import socket
import threading
import unittest
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
from peripheral_sim import serve_socket

class TestBridge(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.thread = threading.Thread(target=serve_socket, kwargs={'port': 18765}, daemon=True)
        cls.thread.start()
        import time; time.sleep(0.05)
    def call(self, req):
        with socket.create_connection(('127.0.0.1',18765),timeout=2) as s:
            s.sendall((json.dumps(req)+'\n').encode())
            return json.loads(s.makefile('rb').readline())
    def test_spi_identity(self):
        r=self.call({'op':'spi','cs':'imu','reg':0x75|0x80,'len':1})
        self.assertEqual(r['data'],[0x47])
    def test_i2c_identity(self):
        r=self.call({'op':'i2c','addr':0x1e,'reg':0x0a,'len':3})
        self.assertEqual(r['data'],[ord('H'),ord('4'),ord('3')])

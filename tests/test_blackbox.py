import subprocess
import unittest
import sys
import httplib
import os
import sys
import itertools
import time
from urlparse import urljoin

exe = os.environ['EXECUTABLE']


class BlackboxTestCase(unittest.TestCase):
    """Test suite that runs every test case on a single
    instance of server
    """
    @classmethod
    def _create_http_connection(cls):
        return httplib.HTTPConnection(host='127.0.0.1', port=5000)

    def setUp(self):
        self.proc = subprocess.Popen([exe])
        self.conn = self._create_http_connection()

    def request(self, *args, **kwargs):
        for retry in xrange(10):
            try:
                self.conn.request(*args, **kwargs)
                res = self.conn.getresponse()
                return res
            except Exception as e:
                sys.stderr.write('Cannot send request ({0}).. retry {1}\n'.format(e, retry))
                time.sleep(0.1)
                self.conn = self._create_http_connection()
                continue
        raise httplib.CannotSendRequest()

    def tearDown(self):
        # The process should stop after receiving GET /cancel/
        res = self.request('GET', '/cancel/')
        self.assertEqual(res.read(), 'success=0\n')
        self.conn.close()
        exit_code = self.proc.wait()
        self.assertEqual(exit_code, 0)

    def test_get(self):
        res = self.request('GET', '/get/')
        self.assertEqual(res.status, 200)
        self.assertEqual(res.read(), 'url=/get/\n')
        self.assertEqual(res.getheader('Transfer-Encoding'), 'chunked')

    def test_invalid_method(self):
        res = self.request('POST', '/get/')
        self.assertEqual(res.status, 405)
        self.assertEqual(res.read(), '')
        self.assertEqual(res.getheader('Transfer-Encoding'), 'chunked')

    def test_post(self):
        res = self.request('POST', '/post/', 'hello world!')
        self.assertEqual(res.status, 200)
        self.assertEqual(res.read(), 'body=hello world!\n')
        self.assertEqual(res.getheader('Transfer-Encoding'), 'chunked')

    def test_not_found(self):
        res = self.request('GET', '/page/not/found/')
        self.assertEqual(res.status, 404)
        self.assertEqual(res.read(), '')

    def test_set_headers(self):
        res = self.request('GET', '/set-headers/')
        self.assertEqual(res.status, 200)
        self.assertEqual(res.read(), 'url=/set-headers/\n')
        self.assertEqual(res.getheader('Key0'), 'Value0')
        self.assertEqual(res.getheader('Key1'), 'Value1')
        self.assertEqual(res.getheader('Key2'), 'Value2')
        self.assertEqual(res.getheader('Key3'), 'Value3')
        self.assertEqual(res.getheader('Key4'), 'Value4')
        self.assertEqual(res.getheader('Key5'), 'Value5')
        self.assertEqual(res.getheader('Key6'), 'Value6')
        self.assertEqual(res.getheader('Key7'), 'Value7')
        self.assertEqual(res.getheader('Key8'), 'Value8')
        self.assertEqual(res.getheader('Key9'), 'Value9')

    def test_get_multiple(self):
        res = self.request('GET', '/get/')
        self.assertEqual(res.status, 200)
        self.assertEqual(res.read(), 'url=/get/\n')
        self.assertEqual(res.getheader('Transfer-Encoding'), 'chunked')

        res = self.request('GET', '/get/')
        self.assertEqual(res.status, 200)
        self.assertEqual(res.read(), 'url=/get/\n')
        self.assertEqual(res.getheader('Transfer-Encoding'), 'chunked')

if __name__ == '__main__':
    unittest.main()

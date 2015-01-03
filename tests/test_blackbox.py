import subprocess
import unittest
import sys
import urllib2
import os
import sys
import itertools
import time
from urlparse import urljoin

exe = os.environ['EXECUTABLE']

def urlopen(*args, **kwargs):
    """Sometimes server process is not ready yet, so we need
    to retry whenever connection refused is happens."""
    for retry_count in range(10):
        try:
            return urllib2.urlopen(*args, **kwargs)
        except urllib2.URLError as e:
            if 'Connection refused' in str(e):
                sys.stderr.write('Retrying connection... {}\n'.format(retry_count))
                time.sleep(0.1)
                continue
            raise

class BlackboxTestCase(unittest.TestCase):
    """Test suite that runs every test case on a single
    instance of server
    """
    #@classmethod
    def setUp(self):
        self.proc = subprocess.Popen([exe])
        self.url = 'http://127.0.0.1:5000'

    #@classmethod
    def tearDown(self):
        # The process should stop after receiving GET /cancel/
        res = urlopen(urljoin(self.url, '/cancel/'))
        self.assertEqual(res.read(), 'success=0\n')
        exit_code = self.proc.wait()
        self.assertEqual(exit_code, 0)

    def test_get(self):
        res = urlopen(urljoin(self.url, '/get/'))
        self.assertEqual(res.read(), 'url=/get/\n')
        self.assertEqual(res.info().getheader('Transfer-Encoding'), 'chunked')

    def test_post(self):
        with self.assertRaisesRegexp(urllib2.HTTPError, r'^HTTP Error 405: Method Not Allowed$') as cm:
            res = urlopen(urljoin(self.url, '/get/'), data='asdf')
        res = cm.exception
        self.assertEqual(res.read(), '')
        self.assertEqual(res.info().getheader('Transfer-Encoding'), 'chunked')

    def test_not_found(self):
        with self.assertRaisesRegexp(urllib2.HTTPError, r'^HTTP Error 404: Not Found$') as cm:
            res = urlopen(urljoin(self.url, '/page/not/found/'))
        self.assertEqual(cm.exception.read(), '')

    def test_set_headers(self):
        res = urlopen(urljoin(self.url, '/set-headers/'))
        self.assertEqual(res.read(), 'url=/set-headers/\n')
        self.assertEqual(res.info().getheader('Key0'), 'Value0')
        self.assertEqual(res.info().getheader('Key1'), 'Value1')
        self.assertEqual(res.info().getheader('Key2'), 'Value2')
        self.assertEqual(res.info().getheader('Key3'), 'Value3')
        self.assertEqual(res.info().getheader('Key4'), 'Value4')
        self.assertEqual(res.info().getheader('Key5'), 'Value5')
        self.assertEqual(res.info().getheader('Key6'), 'Value6')
        self.assertEqual(res.info().getheader('Key7'), 'Value7')
        self.assertEqual(res.info().getheader('Key8'), 'Value8')
        self.assertEqual(res.info().getheader('Key9'), 'Value9')

if __name__ == '__main__':
    unittest.main()

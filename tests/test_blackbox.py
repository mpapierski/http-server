import subprocess
import unittest
import sys
import urllib2
import os
from urlparse import urljoin

exe = os.environ['EXECUTABLE']


class BlackboxTestCase(unittest.TestCase):
    """Test suite that runs every test case on a single
    instance of server
    """
    @classmethod
    def setUpClass(cls):
        cls.proc = subprocess.Popen([exe])
        cls.url = 'http://127.0.0.1:5000'

    @classmethod
    def tearDownClass(cls):
        cls.proc.kill()
        cls.proc.wait()

    def test_get(self):
        res = urllib2.urlopen(urljoin(self.url, '/get/'))
        self.assertEqual(res.read(), 'url=/get/\n')

    def test_post(self):
        with self.assertRaisesRegexp(urllib2.HTTPError, r'^HTTP Error 405: Method Not Allowed$') as cm:
            res = urllib2.urlopen(urljoin(self.url, '/get/'), data='asdf')
        self.assertEqual(cm.exception.read(), '')

    def test_not_found(self):
        with self.assertRaisesRegexp(urllib2.HTTPError, r'^HTTP Error 404: Not Found$') as cm:
            res = urllib2.urlopen(urljoin(self.url, '/page/not/found/'))
        self.assertEqual(cm.exception.read(), '')

if __name__ == '__main__':
    unittest.main()

# Copyright (C) 2005-2012 Canonical Ltd
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

import unittest, os, subprocess, tests
from tempfile import mkstemp

class TestCli(unittest.TestCase):

    def test_save(self):
        subprocess.call(["bin/hudkeywords", self.infile], env={"PYTHONPATH": "."})
        
        with open(self.infile, 'r') as f:
            self.assertMultiLineEqual(f.read(), tests.PO_OUTPUT)

    def test_save_xml(self):
        subprocess.call(["bin/hudkeywords", self.infile, "-x", self.outfile], env={"PYTHONPATH": "."})
        
        with open(self.infile, 'r') as f:
            self.assertMultiLineEqual(f.read(), tests.PO_OUTPUT)
        
        with os.fdopen(self.outhandle, 'r') as f:
            self.assertMultiLineEqual(f.read(), tests.XML_OUTPUT)

    def setUp(self):
        self.maxDiff = None
        self.inhandle, self.infile = mkstemp()
        with os.fdopen(self.inhandle, 'w') as f:
            f.write(tests.INPUT)
        self.outhandle, self.outfile = mkstemp()
        
    def tearDown(self):
        os.remove(self.infile)
        os.remove(self.outfile)


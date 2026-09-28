#!/usr/bin/env python

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


import os
from setuptools import setup

def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

setup(name='hudkeywords',
      version='1.0',
      description='HUD keywords tool',
      license='GNU GPL v2',
      author='Pete Woods',
      author_email='pete.woods@canonical.com',
      scripts=['bin/hudkeywords'],
      packages=['hudkeywords'],
      url='http://launchpad.net/hud',
      long_description=read('README'),
      test_suite='tests'
     )

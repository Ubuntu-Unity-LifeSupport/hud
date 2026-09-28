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

'''
hudkeywords.cli -- shortdesc

hudkeywords.cli is a description

It defines classes_and_methods

'''

import sys
import os

from argparse import ArgumentParser
from argparse import RawDescriptionHelpFormatter

import po

__all__ = []
__version__ = 1.0

class CLIError(BaseException):
    '''Generic exception to raise and log different fatal errors.'''
    def __init__(self, msg):
        super(CLIError).__init__(type(self))
        self.msg = "Error: %s" % msg

def main(argv=None): # IGNORE:C0111
    '''Command line options.'''
    
    if argv is None:
        argv = sys.argv
    else:
        sys.argv.extend(argv)

    program_name = os.path.basename(sys.argv[0])
    program_version = "v%s" % __version__
    program_version_message = '%%(prog)s %s' % (program_version)
    program_shortdesc = 'Simple python tool for creating po and keyword XML files for HUD.'

    try:
        # Setup argument parser
        parser = ArgumentParser(description=program_shortdesc, formatter_class=RawDescriptionHelpFormatter)
        parser.add_argument("pofile", help="The PO file to add hud-keyword: entries to")
        parser.add_argument("-v", "--verbose", dest="verbose", action="count", help="set verbosity level")
        parser.add_argument("-x", "--xmloutput", dest="xmloutput", help="Write out a keyword mapping file into this XML file")
        parser.add_argument('-V', '--version', action='version', version=program_version_message)
        
        # Process arguments
        args = parser.parse_args()
        
        if not os.path.exists(args.pofile):
            raise CLIError("PO file [{}] does not exist.".format(args.pofile))
        
        if args.verbose > 0:
            print("Reading PO file [{}]".format(args.pofile))
        po_file = po.PoFile(args.pofile)
        
        if args.verbose > 0:
            print("Updating PO file [{}]".format(args.pofile))
        po_file.save(args.pofile)
        
        if args.xmloutput:
            if args.verbose > 0:
                print("Writing to XML output file [{}]".format(args.xmloutput))
            po_file.save_xml(args.xmloutput);
        
        return 0
    except KeyboardInterrupt:
        ### handle keyboard interrupt ###
        return 0
    except CLIError, e:
        indent = len(program_name) * " "
        sys.stderr.write(program_name + ": " + e.msg + "\n")
        sys.stderr.write(indent + "  for help use --help\n")
        return 2
    except Exception, e:
        indent = len(program_name) * " "
        sys.stderr.write(program_name + ": " + repr(e) + "\n")
        sys.stderr.write(indent + "  for help use --help\n")
        return 2

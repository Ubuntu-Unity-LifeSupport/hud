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
Created on 10 Dec 2012
'''

import polib, os, sys
from lxml import etree
from lxml.etree import Element, ElementTree, SubElement

class PoFile:
    def __init__(self, infile):
        self.original = polib.pofile(infile)
        self.po = polib.pofile(infile)
        self._add_keyword_entries()

    def _add_keyword_entries(self):
        new_entries = []
        
        for entry in self.po:
            if entry.obsolete:
                continue
            
            # Don't add keyword entries again
            if entry.msgid.startswith(u'hud-keywords:'):
                continue
            
            new_entry = polib.POEntry(
                msgid = u'hud-keywords:{}'.format(entry.msgid),
                msgstr = u'',
                tcomment = entry.tcomment,
                occurrences = entry.occurrences,
                flags = entry.flags,
                previous_msgctxt = entry.previous_msgctxt,
                previous_msgid = entry.previous_msgid,
                previous_msgid_plural = entry.previous_msgid_plural,
            )
            
            # Don't add entries that are already there
            if new_entry not in self.po:
                new_entries.append(new_entry)

        for entry in new_entries:
            self.po.append(entry)
            
    def save(self, path):
        self.po.save(path)
        
    def _read_existing_mappings(self, path):
        existing = {}
        
        if os.path.exists(path) and os.path.getsize(path) > 0:
            keyword_mapping = etree.parse(path).getroot()
            
            for mapping in keyword_mapping.xpath('//keywordMapping/mapping'):
                original = mapping.get('original')
                keywords = []
                for keyword in mapping:
                    keywords.append(keyword.get('name'))
                existing[original] = keywords
            
        return existing
        
    def save_xml(self, path):
        existing = self._read_existing_mappings(path)
            
        keyword_mapping = Element('keywordMapping')
        
        for entry in self.original:
            # No point adding obsolete entries
            if entry.obsolete:
                continue
            
            # Don't add keyword entries again
            if entry.msgid.startswith(u'hud-keywords:'):
                continue
            
            mapping = SubElement(keyword_mapping, 'mapping', original=entry.msgid)
            
            # Either use the old mappings or add some blank ones
            if entry.msgid in existing:
                for keyword in existing[entry.msgid]:
                    SubElement(mapping, 'keyword', name=keyword)
            else:
                SubElement(mapping, 'keyword', name='')
                SubElement(mapping, 'keyword', name='')
            
        ElementTree(keyword_mapping).write(path, encoding='utf-8', xml_declaration=True, pretty_print=True)

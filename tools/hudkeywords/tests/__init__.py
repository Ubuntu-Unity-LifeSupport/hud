
INPUT = r'''# SOME DESCRIPTIVE TITLE.
# Copyright (C) YEAR THE PACKAGE'S COPYRIGHT HOLDER
# This file is distributed under the same license as the PACKAGE package.
# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.
# 
#, fuzzy
msgid ""
msgstr ""
"Project-Id-Version: hellogt 1.2\n"
"Report-Msgid-Bugs-To: \n"
"POT-Creation-Date: 2012-11-21 14:24+0000\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\n"
"Language-Team: LANGUAGE <LL@li.org>\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=CHARSET\n"
"Content-Transfer-Encoding: 8bit\n"
"Language: \n"

#: hellogt.cxx:16
msgid "hello, world!"
msgstr ""

#: hellogt.cxx:17 hellogt.cxx:19
msgid "another string"
msgstr ""

#: hellogt.cxx:20
msgid "another string 2"
msgstr ""
'''
    
PO_OUTPUT = INPUT + r'''
#: hellogt.cxx:16
msgid "hud-keywords:hello, world!"
msgstr ""

#: hellogt.cxx:17 hellogt.cxx:19
msgid "hud-keywords:another string"
msgstr ""

#: hellogt.cxx:20
msgid "hud-keywords:another string 2"
msgstr ""
'''

XML_OUTPUT = r'''<?xml version='1.0' encoding='UTF-8'?>
<keywordMapping>
  <mapping original="hello, world!">
    <keyword name=""/>
    <keyword name=""/>
  </mapping>
  <mapping original="another string">
    <keyword name=""/>
    <keyword name=""/>
  </mapping>
  <mapping original="another string 2">
    <keyword name=""/>
    <keyword name=""/>
  </mapping>
</keywordMapping>
'''

XML_INPUT = r'''<?xml version='1.0' encoding='UTF-8'?>
<keywordMapping>
  <mapping original="another string">
    <keyword name="yo yo another"/>
  </mapping>
</keywordMapping>
'''

XML_OUTPUT_EXISTING = r'''<?xml version='1.0' encoding='UTF-8'?>
<keywordMapping>
  <mapping original="hello, world!">
    <keyword name=""/>
    <keyword name=""/>
  </mapping>
  <mapping original="another string">
    <keyword name="yo yo another"/>
  </mapping>
  <mapping original="another string 2">
    <keyword name=""/>
    <keyword name=""/>
  </mapping>
</keywordMapping>
'''
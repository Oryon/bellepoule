#!/usr/bin/env python
import os
import sys
import requests
import re

launchpad   = 'https://launchpad.net/~betonniere/+archive/ubuntu/'
application = 'bellepoule'
major       = None
minor       = None
phase       = None
maturity    = None
sha256      = None
v           = None

dep_list = ['gtk2>=2.24.0',
            'xml2',
            'curl',
            'libmicrohttpd',
            'goocanvas1',
            'qrencode',
            'openssl',
            'lighttpd',
            'php-cgi',
            'json-glib']

class TargetFile:
    # --------------------------------
    def __init__ (self, template, output_file):
        self.__template    = template
        self.__output_file = output_file

    # --------------------------------
    def generate (self):
        with open (self.__template, 'r') as template:
            content = template.read ()
            content = content.replace ('#APPLICATION', application)
            content = content.replace ('#PHASE',       phase)
            content = content.replace ('#MAJOR',       major)
            content = content.replace ('#MINOR',       minor)
            content = content.replace ('#V',           v)
            content = content.replace ('#MATURITY',    maturity)
            content = content.replace ('#DEPS',        self.get_deps_image ())
            content = content.replace ('#SHA256',      sha256)

            with open (self.__output_file, 'w') as output:
                output.write (content)
                output.close ()

    # --------------------------------
    def get_deps_image (self):
        return 'get_deps_image ==> Not implemented'

class PkgbuildFile (TargetFile):
    # --------------------------------
    def __init__ (self, out):
        TargetFile.__init__ (self, 'PKGBUILD.tpl', out)

    # --------------------------------
    def get_deps_image (self):
        deps_image = ''

        for dep in dep_list:
            if deps_image != '':
                deps_image = deps_image + ' '
            deps_image = deps_image + "'" + dep + "'"

        return deps_image

class SrcInfoFile (TargetFile):
    # --------------------------------
    def __init__ (self, out):
        TargetFile.__init__ (self, 'SRCINFO.tpl', out)

    # --------------------------------
    def get_deps_image (self):
        deps_image = ''
        eol        = None

        for dep in dep_list:
            if eol:
                deps_image = deps_image + eol
            deps_image = deps_image + '	depends = ' + dep
            eol = '\n'

        return deps_image

# --------------------------------
def get_sha256 ():
    dsc_uri  = launchpad
    dsc_uri += application
    dsc_uri += '/+files/'
    dsc_uri += application
    dsc_uri += phase + '_'
    dsc_uri += major
    dsc_uri += '.'
    dsc_uri += minor
    dsc_uri += 'ubuntu' + maturity + '~xenial'
    dsc_uri += v
    dsc_uri += '.dsc'

    print dsc_uri
    ubuntu_dsc = requests.get (dsc_uri)
    target = re.search (r'Checksums-Sha256:.*Files:', ubuntu_dsc.content, re.S)
    if target:
        segments = target.group(0).split ()
        if len (segments) > 2:
            return segments[1]

    return None

# --------------------------------
if __name__ == '__main__':
    if getattr (sys, 'frozen', False):
        os.chdir (os.path.dirname (sys.executable))
    else:
        os.chdir (os.path.dirname (os.path.realpath (__file__)))

    if len (sys.argv) is 5:
        major    = sys.argv[1]
        minor    = sys.argv[2]
        v        = sys.argv[3]
        maturity = sys.argv[4]
        phase    = 'beta'
        #phase    = ''

    sha256 = get_sha256 ()

    target = SrcInfoFile ('.SRCINFO')
    target.generate ()

    target = PkgbuildFile ('PKGBUILD')
    target.generate ()

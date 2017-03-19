#!/usr/bin/env python
import os
import sys
import requests
import re

launchpad   = 'https://launchpad.net/~betonniere/+archive/ubuntu/'
application = 'bellepoule'
major       = '4'
minor       = '31'
ubuntu      = 'ubuntu1~xenial1'

if __name__ == '__main__':
    if getattr (sys, 'frozen', False):
        os.chdir (os.path.dirname (sys.executable))
    else:
        os.chdir (os.path.dirname (os.path.realpath (__file__)))

    dsc_file  = launchpad
    dsc_file += application
    dsc_file += '/+files/'
    dsc_file += application
    dsc_file += '_'
    dsc_file += major
    dsc_file += '.'
    dsc_file += minor
    dsc_file += ubuntu
    dsc_file += '.dsc'

    dsc = requests.get (dsc_file)

    target = re.search (r'Checksums-Sha256:.*Files:', dsc.content, re.S)
    if target:
        segments = target.group(0).split ()
        if len (segments) > 2:
            sha256 = segments[1]

    with open ('PKGBUILD.tpl', 'r') as template:
        pkgbuild = template.read ()
        pkgbuild = pkgbuild.replace ('__APPLICATION__', application)
        pkgbuild = pkgbuild.replace ('__MAJOR__',       major)
        pkgbuild = pkgbuild.replace ('__MINOR__',       minor)
        pkgbuild = pkgbuild.replace ('__SHA256__',      sha256)

        with open ('PKGBUILD', 'w') as output:
            output.write (pkgbuild)
            output.close ()

            print 'mksrcinfo'

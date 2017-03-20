#!/usr/bin/env python
import os
import sys
import requests
import re

launchpad   = 'https://launchpad.net/~betonniere/+archive/ubuntu/'
application = 'bellepoule'
major       = None
minor       = None
maturity    = None
sha256      = None

# --------------------------------
def generate (tpl, out):
    with open (tpl, 'r') as template:
        content = template.read ()
        content = content.replace ('__APPLICATION__', application)
        content = content.replace ('__MAJOR__',       major)
        content = content.replace ('__MINOR__',       minor)
        content = content.replace ('__SHA256__',      sha256)

        with open (out, 'w') as output:
            output.write (content)
            output.close ()

# --------------------------------
def get_version (tag):
    with open ('../sources/BellePoule/application/version.h', 'r') as version_file:
        content = version_file.read ()
        line = re.search (r'#define ' +tag + ' .*', content)
        if line:
            segments = line.group(0).split ()
            return segments[2].replace ('"', '')

    return None

# --------------------------------
def get_sha256 ():
    dsc_uri  = launchpad
    dsc_uri += application
    dsc_uri += '/+files/'
    dsc_uri += application
    dsc_uri += 'beta_'
    dsc_uri += major
    dsc_uri += '.'
    dsc_uri += minor
    dsc_uri += 'ubuntu' + maturity + '~xenial1'
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

    major    = get_version ("VERSION")
    minor    = get_version ("VERSION_REVISION")
    maturity = get_version ("VERSION_MATURITY").replace ('alpha', '')
    sha256   = get_sha256  ()

    generate ('SRCINFO.tpl',  '.SRCINFO')
    generate ('PKGBUILD.tpl', 'PKGBUILD')

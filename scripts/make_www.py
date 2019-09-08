#!/usr/bin/env python

import os
import sys
import argparse
import zipfile

class Color:
  RED     ='\033[1;31m'
  GREEN   ='\033[1;32m'
  YELLOW  ='\033[1;33m'
  BLUE    ='\033[1;34m'
  MAGENTA ='\033[1;35m'
  CYAN    ='\033[1;36m'
  WHITE   ='\033[0;37m'
  END     ='\033[0m'
  KO = RED   + 'KO' + END
  OK = GREEN + 'OK' + END

# -------------------------------------------------
def parse_args ():
    parser = argparse.ArgumentParser (description='www folder maker.')
    parser.add_argument ('-v', '--verbose', action='store_true')

    args = parser.parse_args ()

# -------------------------------------------------
def excluded (name):
    if name.endswith ('.cotcot'):
        return True

    if name.startswith ('cognac/.git'):
        return True

    return False

# -------------------------------------------------
if __name__ == '__main__':
    parse_args   ()

    if getattr (sys, 'frozen', False):
        os.chdir (os.path.dirname (sys.executable) + '/../sources/www')
    else:
        os.chdir (os.path.dirname (os.path.realpath (__file__)) + '/../sources/www')

    with zipfile.ZipFile ('../../resources/www.zip', 'w', zipfile.ZIP_DEFLATED) as zip:
        root = 'cognac'
        for (dirpath, dirnames, filenames) in os.walk (root):
            for file in filenames:
                if file.endswith ('.cotcot'):
                    continue
                if file.startswith ('.git'):
                    continue

                arcname = os.path.join (dirpath, file)
                arcname = arcname.replace ('cognac/', 'www/')
                zip.write (os.path.join (dirpath, file),
                           arcname)

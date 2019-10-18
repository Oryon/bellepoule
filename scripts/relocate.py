#!/usr/bin/env python

import os
import argparse

args              = None
install_name_tool = os.environ['HOME'] + '/Project/Cross/Osxcross/build/cctools-port/cctools/misc/install_name_tool'

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
    global checks
    global args

    parser = argparse.ArgumentParser ()
    parser.add_argument ('-d', '--dependencies-path', action='store', help='dependencies(.dylib) path');
    parser.add_argument ('targets', type=str, nargs='*', help='target libs to relocate')

    args = parser.parse_args ()

# -------------------------------------------------
def relocatable (name):
    if name.endswith ('.dylib'):
        return True
    if name.endswith ('.so'):
        return True

    return False

# -------------------------------------------------
if __name__ == '__main__':
    parse_args ()

    if len (args.targets) == 0:
        args.targets = os.listdir ('.')

    if args.dependencies_path is None:
        args.dependencies_path = ''

    # -id
    for target in args.targets:
        if relocatable (target):
            os.system (install_name_tool+' -id @executable_path/../Frameworks/'+target+' '+target)

    # -change
    for target in args.targets:
        if relocatable (target):
            for lib in os.listdir ('./' + args.dependencies_path):
                if relocatable (lib):
                    os.system (install_name_tool+' -change /opt/local/lib/'+lib+' @executable_path/../Frameworks/'+lib+' '+target)

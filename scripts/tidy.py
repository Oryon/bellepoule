#!/usr/bin/env python

import os
import sys
import re
import argparse
import subprocess

checks = ''

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

    parser = argparse.ArgumentParser ()
    parser.add_argument ('-o', '--override', action='store_true', help='modernize-use-override');
    parser.add_argument ('-n', '--null',     action='store_true', help='modernize-use-nullptr')
    parser.add_argument ('-d', '--default',  action='store_true', help='modernize-use-default')
    parser.add_argument ('-f', '--fix',      action='store_true', help='fix')

    args = parser.parse_args ()

    checks    = " -checks='"
    separator = ''
    if args.override:
        checks += separator + 'modernize-use-override'
        separator = ','
    if args.null:
        checks += separator + 'modernize-use-nullptr'
        separator = ','
    if args.default:
        checks += separator + 'modernize-use-default'
        separator = ','
    checks += "'"

    if args.fix:
        checks += ' -fix'

# -------------------------------------------------
if __name__ == '__main__':
    parse_args ()

    if getattr (sys, 'frozen', False):
        os.chdir (os.path.dirname (sys.executable) + '/../build/BellePoule')
    else:
        os.chdir (os.path.dirname (os.path.realpath (__file__)) + '/../build/BellePoule')

    for path, dirs, files in os.walk ('../../sources'):
        if path.endswith ('LivePoule') is False:
            print '\n' + Color.BLUE + path + Color.END
            for f in files:
                if f.endswith ('.cpp') or f.endswith ('.hpp'):
                    print '   ' + f

                    cmd = 'clang-tidy'

                    cmd += checks
                    cmd += ' ' + path + '/' + f
                    cmd += ' -- -std=c++11'
                    cmd += ' -DDEBUG -DCODE_BLOCKS=1 -DGTK_DISABLE_SINGLE_INCLUDES -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGSEAL_ENABLE -DWEBKIT'
                    cmd += ' -I/usr/include/libxml2'
                    cmd += ' -I/usr/include/goocanvas-2.0'
                    cmd += ' -I/usr/include/gtk-3.0'
                    cmd += ' -I/usr/include/at-spi2-atk/2.0'
                    cmd += ' -I/usr/include/at-spi-2.0'
                    cmd += ' -I/usr/include/dbus-1.0'
                    cmd += ' -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include'
                    cmd += ' -I/usr/include/gio-unix-2.0/'
                    cmd += ' -I/usr/include/mirclient'
                    cmd += ' -I/usr/include/mircommon'
                    cmd += ' -I/usr/include/mircookie'
                    cmd += ' -I/usr/include/cairo'
                    cmd += ' -I/usr/include/pango-1.0'
                    cmd += ' -I/usr/include/harfbuzz'
                    cmd += ' -I/usr/include/atk-1.0'
                    cmd += ' -I/usr/include/gdk-pixbuf-2.0'
                    cmd += ' -I/usr/include/libpng12'
                    cmd += ' -I/usr/include/glib-2.0'
                    cmd += ' -I/usr/lib/x86_64-linux-gnu/glib-2.0/include'
                    cmd += ' -I/usr/include/pixman-1'
                    cmd += ' -I/usr/include/freetype2'
                    cmd += ' -I/usr/include/p11-kit-1'
                    cmd += ' -I/usr/include/webkitgtk-1.0'
                    cmd += ' -I/usr/include/libsoup-2.4'
                    cmd += ' -I../../sources/BellePoule'
                    cmd += ' -I../../sources/common'
                    cmd += ' -I/usr/include/json-glib-1.0'
                    cmd += ' -I../../sources/common/network'

                    p = subprocess.Popen (cmd, shell=True, stdout=subprocess.PIPE)
                    stdout = p.stdout.read ()

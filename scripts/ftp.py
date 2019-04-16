#!/usr/bin/env python
# coding: utf-8

import argparse
import os
import sys
import getpass
import stat
import logging
import signal
import atexit
import subprocess
import libxml2

from hashlib           import md5
from xdg.BaseDirectory import xdg_config_home
from xdg.BaseDirectory import xdg_data_home

from pyftpdlib.handlers    import FTPHandler
from pyftpdlib.handlers    import FTPHandler
from pyftpdlib.servers     import FTPServer
from pyftpdlib.authorizers import DummyAuthorizer
from pyftpdlib.authorizers import AuthenticationFailed

args = None
www  = None

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
    global args

    parser = argparse.ArgumentParser ()
    parser.add_argument ('-c', '--create-user', action='store_true', help='Create the user account.');
    parser.add_argument ('-v', '--verbose',     action='store_true', help='Verbosity.');

    args = parser.parse_args ()

    if args.verbose:
        logging.basicConfig (level=logging.DEBUG)

# -------------------------------------------------
class SmartAuthorizer (DummyAuthorizer):

    def validate_authentication (self, username, password, handler):
        password = md5 (password.encode ('latin1'))
        hash = password.hexdigest ()
        try:
            if self.user_table[username]['pwd'] != hash:
                raise KeyError
        except KeyError:
            raise AuthenticationFailed


# -------------------------------------------------
class SmartHandler (FTPHandler):

    def on_file_received (self, cotcot):
        props = ['Arme', 'Sexe', 'Categorie', 'Label']

        with open (cotcot) as f:
            doc = libxml2.parseDoc (f.read ())
            root = doc.children

            folder = None
            current = root.next
            while current:
                if  current.name != 'comment':
                    for p in props:
                        prop = current.prop (p)
                        if prop:
                            if folder:
                                folder += '-' + prop
                            else:
                                folder = prop
                    break
                current = current.next

            doc.freeDoc ()

            if folder:
                location = www + '/' + folder + '/cotcot'
                if not os.path.exists (location):
                    os.makedirs (location)
                os.rename (cotcot, location + '/' + os.path.basename (cotcot))
            else:
                os.remove (cotcot)


# -------------------------------------------------
if __name__ == '__main__':
    global www

    parse_args ()

    #
    credentials = xdg_config_home + '/BellePoule/ftpd.user'
    if args.create_user or os.path.isfile (credentials) is False:
        password = getpass.getpass ("Please assign a user's password: ")
        with open (credentials, 'w') as f:
            password = md5 (password.encode ('latin1'))
            f.write (password.hexdigest ())

    with open (credentials, 'r') as f:
        password = f.read ()
        os.chmod (credentials, stat.S_IREAD| stat.S_IWRITE)

    #
    www = xdg_data_home + '/BellePoule/www'

    #
    php_launcher = 'php --server 0.0.0.0:8000 --docroot ' + www
    php = subprocess.Popen (php_launcher, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    #
    authorizer = SmartAuthorizer ()
    authorizer.add_user      ('bellepoule', password, www, perm='lre' + 'wd')
    authorizer.add_anonymous (www, perm='lre')

    handler = SmartHandler
    handler.authorizer = authorizer

    server = FTPServer (('127.0.0.1', 1026), handler)
    server.serve_forever ()

#!/usr/bin/env python

import os
import ConfigParser
import lxml.html
import io
import zipfile
import argparse
import xml.etree.ElementTree as ElementTree
import xml.dom.minidom as minidom
import requests
try:
 requests.packages.urllib3.disable_warnings()
except:
  pass

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

xml_path    = None
cotcot_path = None
config      = None

# -------------------------------------------------
def parse_args ():
    global xml_path
    global cotcot_path

    parser = argparse.ArgumentParser (description='Fencing file convertor.')
    parser.add_argument ('-v',  '--verbose', action='store_true')
    parser.add_argument ('xml')

    args = parser.parse_args ()
    if args.xml:
        xml_path    = args.xml
        cotcot_path = xml_path.replace ('.xml', '[Fresh].cotcot')

# -------------------------------------------------
def parse_config ():
    global config

    config = ConfigParser.RawConfigParser ()
    config.read (os.environ['HOME'] + '/.ffe')

# -------------------------------------------------
def extract_ranks (path):
    ranks = {}
    tree  = ElementTree.parse (path)
    root  = tree.getroot ()

    tireurs = root.find ('./Tireurs')
    for tireur in tireurs:
        if tireur.attrib['Classement'] != "":
            key        = tireur.attrib['Nom'] + '_' + tireur.attrib['Prenom']
            ranks[key] = tireur.attrib['Classement']
        else:
            print (Color.RED + tireur.attrib['Nom'] + Color.END + ' has no "Classement"')

    return ranks

# -------------------------------------------------
def inject_ranks (path, ranks):
    tree = ElementTree.parse (path)
    root = tree.getroot ()

    if root.tag == 'CompetitionIndividuelle':
        tireurs = root.find ('./Tireurs')
        for tireur in tireurs:
            key = tireur.attrib['Nom'] + '_' + tireur.attrib['Prenom']
            if key in ranks:
                tireur.set ('Ranking', ranks[key])
            else:
                print (Color.RED + tireur.attrib['Nom'] + Color.END + ' has no "Ranking"')
                tireur.set ('Ranking', '0')

        poules = root.find ('./Phases/TourDePoules')
        if poules is not None:
            poules.set ('Decalage', '/region/club/')

    tree.write (path.replace ('.xml', '[1].cotcot'))
    tree.write (path.replace ('.xml', '[2].cotcot'))

# -------------------------------------------------
def get_competition_id (path):
    tree = ElementTree.parse (path)
    root = tree.getroot ()

    if root.tag == 'CompetitionIndividuelle':
        return (root.attrib['ID'])

# -------------------------------------------------
def prettify (elem):
    rough_string = ElementTree.tostring (elem, 'iso-8859-1')
    reparsed     = minidom.parseString (rough_string)

    return reparsed.toprettyxml (indent='  ', encoding='iso-8859-1')

# -------------------------------------------------
if __name__ == '__main__':
    os.unsetenv ('http_proxy')
    os.unsetenv ('https_proxy')

    parse_args   ()
    parse_config ()

    competition_id = get_competition_id (xml_path)

    s = requests.session ()

    login = s.get ('https://extranet.escrime-ffe.fr')

    login_html    = lxml.html.fromstring (login.text)
    hidden_inputs = login_html.xpath (r'//form//input[@type="hidden"]')
    form          = {x.attrib['name']: x.attrib['value'] for x in hidden_inputs}

    form['signin[username]'] = config.get ('FFE', 'user')
    form['signin[password]'] = config.get ('FFE', 'password')

    response = s.post ('https://extranet.escrime-ffe.fr/login',
                       data    = form)
    if response.status_code == 200:
        response = s.get ('https://extranet.escrime-ffe.fr/competition/downloadInscrits?id=' + competition_id,
                          stream  = True)
        if response.status_code == 200:
            with zipfile.ZipFile (io.BytesIO (response.content)) as ffe_zip:
                for zipinfo in ffe_zip.infolist ():
                    if zipinfo.filename.endswith ('.XML'):
                        with ffe_zip.open (zipinfo) as ffe_file:
                            ranks = extract_ranks (ffe_file)
                            inject_ranks (xml_path, ranks)
                            break

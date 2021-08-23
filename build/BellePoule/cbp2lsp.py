#!/usr/bin/env python

import sys
import os
from lxml import etree

# -------------------------------------------------
if __name__ == '__main__':
    if getattr (sys, 'frozen', False):
        build_dir = os.path.dirname (sys.executable)
    else:
        build_dir = os.path.dirname (os.path.realpath (__file__))

    tree = etree.parse('BellePoule.cbp')

    options     = ['-Wunused-private-field']
    directories = []
    filenames   = []

    for add in tree.xpath('/CodeBlocks_project_file/Project/Compiler/Add'):
        option = add.get('option')
        if option:
            options.append(option)

        directory = add.get('directory')
        if directory:
            directories.append(directory)

    for unit in tree.xpath('/CodeBlocks_project_file/Project/Unit'):
        filename = unit.get('filename')
        if filename.endswith('.cpp'):
            filenames.append(filename)

    with open('../../sources/compile_commands.json', 'w') as file:
        file.write('[\n')

        for filename in filenames:
            file.write('{\n')
            file.write('  "directory": "' + build_dir + '",\n')

            file.write('  "command": ')
            file.write('"gcc ')
            for option in options:
                file.write(option + ' ')
            for directory in directories:
                file.write('-I' + directory + ' ')
            file.write('-c ' + filename)
            file.write('",\n')

            file.write('  "file": "' + filename + '"\n')
            file.write('},\n')

        file.write(']\n')

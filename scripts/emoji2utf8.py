import sys

if len (sys.argv) == 2:
    uni  = unichr (int (sys.argv[1], 16))
    utf8 = uni.encode ('utf8')
    code_array = bytearray (utf8)

    string = ''
    for code in code_array:
        string += '\\x' + format (code, '02x')

    print string

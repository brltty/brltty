#!/usr/bin/python

import re
import sys

# haha!
myre = re.compile('<HLP> (\w+):(?: "(.+?)")?? : "(.+?)" </HLP>')
# \w is any alphanumeric, (?:...) contains stuff without forming a
# match group, +? and ?? are non-greedy forms of + and ?.

# To change '" / "' to ' / ' for HKEY2 and PHKEY2
key_unq = re.compile('" / "')

# To change 'DOT1|DOT2' to 'DOT1DOT2'
key_chord = re.compile('(DOT[1-6])\\|(?=DOT)')
# (?=...) is a look-ahead assertion (does not consume the text).

# To remove K_
key_prfx = re.compile('K_|DOT')

# To change '|' to '+'
key_bar = re.compile('\\|')

# To change '" "' to ''
unq = re.compile('" *"')

lines = sys.stdin.readlines()

def concat(a,b):
    if a and a[-1] == '\n':
        a = a[:-1]
    if a and a[-1] == '\r':
        a = a[:-1]
    return a+b

txt = reduce(concat, lines, '')

matches = myre.findall(txt)
for n,k,d in matches:
    if k:
        k = key_unq.sub(' / ', k)
        k = key_chord.sub('\\1', k)
        k = key_prfx.sub('', k)
        k = key_bar.sub('+', k)
        k = unq.sub('', k)
    d = unq.sub('', d)
    if k:
        sys.stdout.write(n+':'+k+': '+d+'\n')
    else:
        sys.stdout.write(n+':'+d+'\n')

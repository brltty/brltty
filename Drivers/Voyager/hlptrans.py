#!/usr/bin/python

import re
import sys

re_helpLine = re.compile('<HLP> (\w+):(?: "(.+?)")?? : "(.+?)" </HLP>')
# \w is any alphanumeric
# (?:...) contains stuff without forming a # match group
# +? and ?? are non-greedy forms of + and ?

# To change 'DOT1|DOT2' to 'DOT1DOT2'
re_adjacentDots = re.compile('(DOT[1-8])\\|(?=DOT)')
# (?=...) is a look-ahead assertion (does not consume the text)

# To remove key name prefixes
re_keyPrefixes = re.compile('K_|DOT')

# To change '|' to '+'
re_keyDelimiter = re.compile('\\|')

# To change '" "' to ''
re_adjacentStrings = re.compile('" *"')

# To  find common prefixes in key pairs
re_keyPair = re.compile('^((?P<prefix>.*) +)?((?P<common>.*\\+)(?P<first>.*))/(\\4(?P<second>.*))')

def concatenateLines(first, second):
    if first and first[-1] == '\n':
        first = first[:-1]
    if first and first[-1] == '\r':
        first = first[:-1]
    return first + second

inputLines = sys.stdin.readlines()
helpText = reduce(concatenateLines, inputLines, '')
helpLines = re_helpLine.findall(helpText)
for where,keys,description in helpLines:
    if keys:
        keys = re_adjacentDots.sub('\\1', keys)
        keys = re_keyPrefixes.sub('', keys)
        keys = re_keyDelimiter.sub('+', keys)
        keys = re_adjacentStrings.sub('', keys)
        pair = re_keyPair.search(keys)
        if pair:
            keys = pair.group('common') + ' ' + pair.group('first') + '/' + pair.group('second')
            prefix = pair.group('prefix')
            if prefix:
                keys = prefix + keys
    description = re_adjacentStrings.sub('', description)
    if keys:
        sys.stdout.write(where + ':' + keys + ': ' + description + '\n')
    else:
        sys.stdout.write(where + ':' + description + '\n')

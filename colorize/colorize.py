import sys
import getopt
import re

def strtoreflags(s):
    flags = 0
    for c in s:
        if c == 'i':
            flags |= re.I
    return flags

def usage():
    progname = sys.argv[1:].split('/')[-1]
    print 'usage: %s <colorize_script>' % progname

def main(args):
    (opts, args) = getopt.getopt(args, 'h')
    opts = dict(opts)
    if 'h' in opts or not args:
        usage()
        return
    colorcodes = {
        'red' : '\033[31m',
        'yellow' : '\033[33m',
        'green' : '\033[32m',
        'RED' : '\033[1;31m',
        'YELLOW' : '\033[1;33m',
        'GREEN' : '\033[1;32m',
    }
    rules = []
    with open(args[0], 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            pos = line.rindex(' ')
            if pos == -1:
                continue
            regex = line[:pos].strip()
            color = line[pos+1:].strip()

            pos = regex.rindex('/')
            regexflags = regex[pos+1:]
            regex = regex[1:pos]

            regexflags = strtoreflags(regexflags)
            regex = re.compile(regex, regexflags)

            rules.append((regex, color))

    for line in sys.stdin:
        line = line[:-1] # remove \n
        matched = False
        for (regex, color) in rules:
            match = regex.match(line)
            if match:
                print '%s%s\033[0m' % (colorcodes[color], line)
                matched = True
                break
        if not matched:
            print line

if __name__ == '__main__':
    main(sys.argv[1:])

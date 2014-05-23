#!/usr/bin/python

import sys

import proto

def gen_payload(size):
    s = ''
    while size > 0:
        s += 'a'
        size -= 1
    return s

def main():
    for i in range(0, 0x100):
        hdr = proto.Header()
        hdr.type = i
        hdr.len = 256 + i
        payload = gen_payload(hdr.len)
        sys.stdout.write(str(hdr))
        sys.stdout.write(payload)

if __name__ == '__main__':
    main()

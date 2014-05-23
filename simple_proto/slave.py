#!/usr/bin/python

import sys

import proto

def hex_dump(bytes):
    s = ''
    for b in bytes:
        s += '%02x' % ord(b)
    return s

def main():
    with open('master.log', 'w') as log_file:
        while True:
            bytes = sys.stdin.read(proto.Header.SIZE)
            if not bytes:
                break
            hdr = proto.Header(bytes)
            log_file.write('Received header:\n Raw bytes: %s\n Type: 0x%x\n Len:  %d\n' % (hex_dump(bytes), hdr.type, hdr.len))
            if hdr.len > 0:
                payload = sys.stdin.read(int(hdr.len))
                log_file.write('Received payload\n')
        log_file.close()

if __name__ == '__main__':
    main()

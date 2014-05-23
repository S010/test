#!/usr/bin/python

class Header:
    SIZE = 3

    def __init__(self, bytes = None):
        if bytes:
            self.type = ord(bytes[0])
            self.len = (ord(bytes[1]) << 8) + ord(bytes[2])
        else:
            self.type = 0
            self.len = 0

    def __str__(self):
        s = ''
        s += chr(self.type & 0xff)
        s += chr((self.len >> 8) & 0xff)
        s += chr(self.len & 0xff)
        return s

def compute_chksum(data):
    chksum = 0
    for byte in data:
        chksum <<= 1
        chksum ^= ord(byte)
        chksum &= 0xff
    return chksum

def main():
    data = '\x01\x00\x00'
    print '0x%02x' % compute_chksum(data)

if __name__ == '__main__':
    main()

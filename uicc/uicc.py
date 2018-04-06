#!/usr/bin/python

import serial # PySerial
import getopt
import sys

BIT_1 = 1 << 0
BIT_2 = 1 << 1
BIT_3 = 1 << 2
BIT_4 = 1 << 3
BIT_5 = 1 << 4
BIT_6 = 1 << 5
BIT_7 = 1 << 6
BIT_8 = 1 << 7

class UICC:

    def __init__(self, path='/dev/ttyUSB0'):
        self._s = serial.Serial(path, 9600, timeout=1, rtscts=0, dsrdtr=0, xonxoff=0)
        self._s.rts = False
        self._s.dtr = False
        self._s.dtr = True
        self._do_ATR()
        self._do_PPS()

    def _do_ATR(self):
        atr = self._s.read(33) # TS + up to 32 bytes
        if len(atr) == 0:
            raise Exception("failed to read UICC's Answer-to-Reset")
        print 'ATR: %s' % atr.encode("hex")
        self._atr = [ord(x) for x in atr]
        if self._atr[0] != 0x3b:
            raise Exception("FIXME: can't deal with indirect convention")

    def _do_PPS(self):
        if self._is_TA2_present():
            raise Exception("FIXME: TA2 present, can't use default mode")
        pps = "\xff\x00\xff"
        self._s.write(pps)
        data = self._s.read(32)
        print "PPS response: %s" % data.encode("hex")
        #if data != pps:
        #    raise Exception("PPS failed, got response %s" % data.encode("hex"))

    def _is_TA2_present(self):
        if not (self._atr[1] & BIT_8):
            return False
        td_pos = 0
        for bit in (BIT_5, BIT_6, BIT_7, BIT_8):
            if self._atr[1] & bit:
                td_pos += 1
        if self._atr[td_pos+1] & BIT_5:
            return True
        return False

    def STATUS(self):
        self._s.write("\x80\xf2\x00\x00\x00")
        return self._s.read(0xfe)


def _usage():
    print "usage: %s </path/to/device>" % sys.argv[0]
    print ""
    print "Driver program for a USB UICC (GSM/LTE SIM card) adapter."

if __name__ == '__main__':
    opts, args = getopt.getopt(sys.argv[1:], "h")
    for opt in opts:
        if opt[0] == '-h':
            _usage()
            sys.exit(0)
        else:
            _usage()
            sys.exit(1)
    if len(args) == 0:
        _usage()
        sys.exit(1)
    path = args[0]

    uicc = UICC(path)
    data = uicc.STATUS()
    print data.encode("hex")
        

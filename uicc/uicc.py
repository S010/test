import serial # PySerial

s = serial.Serial('/dev/ttyUSB0', 9600, timeout=1, rtscts=0, dsrdtr=0, xonxoff=0)
print 'Opened serial port %s' % (s.name)
s.rts = False
s.dtr = False
s.dtr = True
print 'Reset UICC'
data = s.read(32)
print data.encode('hex')

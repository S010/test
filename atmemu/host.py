import sys, socket, os, json

class AtmHost:
    def __init__(self, addr, port):
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.bind((addr, port))
        self.s.listen(0)

    def accept_connections(self):
        conn = None
        addr = None
        while True:
            (conn, addr) = self.s.accept()
            pid = os.fork()
            if pid > 0:
                break
        self.conn = conn
        self.addr = addr
        self.control_atm()

    def control_atm(self):
        self.conn.send(json.dumps({ 'type' : 'command', 'command' : 'test_command' }))
        self.conn.shutdown(socket.SHUT_RDWR)
        self.conn.close()

if __name__ == '__main__':
    atmhost = AtmHost(sys.argv[1], int(sys.argv[2]))
    atmhost.accept_connections()

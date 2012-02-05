import socket, json

class Atm:
    def __init__(self):
        self.cassettes = [Cassette(), Cassette(), Cassette(), Cassette()]

    def load_money(self, cassette_no, ccy, denom, n):
        self.cassettes[cassette_no].ccy = ccy
        self.cassettes[cassette_no].denom = denom
        self.cassettes[cassette_no].n = n

    def connect_to_host(self, addr, port):
        self.conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.conn.connect((addr, port))

        while True:
            msg = self.conn.recv(4096)
            print 'Received message from host: %s' % repr(msg)
            self.process_host_msg(msg)
    
    def process_host_msg(self, msg):
        msg = json.loads(msg)

        if msg['type'] == 'command':
            print 'A command msg: %s' % msg['command']

    def go_in_service(self):
        pass

class Cassette:
    def __init__(self):
        pass

class Dispenser:
    def __init__(self):
        pass

class Pinpad:
    def __init__(self):
        pass

class Screen:
    def __init__(self):
        pass

class Opkeys:
    def __init__(self):
        pass

class ControlPanel:
    def __init__(self):
        pass

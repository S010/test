from devices import *

class DieboldAtm(Atm):
    def __init__(self):
        self.cassettes = [Cassette(), Cassette(), Cassette(), Cassette()]

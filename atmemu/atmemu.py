import sys
from devices import *

def main():
    atm = Atm()

    atm.load_money(0, 478, 500, 100)
    atm.load_money(1, 478, 1000, 100)
    atm.load_money(2, 478, 2000, 100) 
    atm.load_money(3, 478, 5000, 100)

    atm.connect_to_host(sys.argv[1], int(sys.argv[2]))

if __name__ == '__main__':
    main()

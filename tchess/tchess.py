#!/usr/bin/python

import time
import curses as c

w = None

def main():
    global w
    w = c.initscr()
    c.noecho()
    c.cbreak()
    w.clear()
    w.addstr(0, 0, 'aaa')
    w.refresh()

    time.sleep(1)

    c.nocbreak()
    c.echo()
    c.endwin()

if __name__ == '__main__':
    main()

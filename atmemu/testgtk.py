#!/usr/bin/python

import pygtk
pygtk.require('2.0')
import gtk

import sys, socket

class Test:
    def on_delete(self, widget, event, data = None):
        # Tell GTK to emit destroy signal
        return False

    def on_destroy(self, widget, data = None):
        gtk.main_quit()

    def hello(self, widget, data = None):
        print 'Hello'

    def __init__(self):
        self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        self.window.connect('delete_event', self.on_delete)
        self.window.connect('destroy', self.on_destroy)
        self.window.set_border_width(10)
        
        self.button = gtk.Button('Hello, world!')
        self.button.connect('clicked', self.hello, None)
        self.button.connect_object('clicked', gtk.Widget.destroy, self.window)

        self.window.add(self.button)

        self.window.show_all()

    def main(self):
        gtk.main()


if __name__ == '__main__':
    test = Test()
    test.main()

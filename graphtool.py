import sys
import pygtk
pygtk.require('2.0')
import gtk

# Data structures

class Graph:
    def __init__(self):
        self.label = 'G'
        self.E = []
        self.V = []

    def add_vertex(self, x = 0, y = 0, label = None):
        if label is None:
            label = 'v%s' % (len(self.V) + 1)
        v = Vertex()
        v.x = x
        v.y = y
        v.label = label
        self.V.append(v)
        return v

    def add_edge(self, V, is_directed = False, weight = 0, label = None):
        if label is None:
            label = 'e%s' % (len(self.E) + 1)
        e = Edge()
        e.label = label
        e.V = V
        e.weight = weight
        e.is_directed = is_directed
        self.E.append(e)
        return e

class Edge:
    def __init__(self):
        self.V = []
        self.weight = 0.0
        self.is_directed = False

class Vertex:
    def __init__(self):
        self.x = 0
        self.y = 0
        self.label = 'v'
        self.edges = []

class IncidenceMatrix:
# rows are edges, cols are vertices
    def __init__(self):
        self.n_rows = 0
        self.n_cols = 0
        self.M = []
        self.graph_fmt = '[%s]'
        self.row_fmt = '%s'
        self.elem_fmt = '%s,'

    def from_graph(self, G):
        self.n_rows = len(G.E)
        self.n_cols = len(G.V)
        self.M = [0 for x in range(self.n_rows * self.n_cols)]

        for i in range(self.n_rows):
            for j in range(self.n_cols):
                e = G.E[i]
                v = G.V[j]
                idx = i * self.n_cols + j

                if e.is_directed:
                    if v in G.V:
                        if G.V[0] == v:
                            self.M[idx] = -1
                        else:
                            self.M[idx] = 1
                else:
                    self.M[idx] = e.V.count(v)

    def __getitem__(self, key):
        (row, col) = key
        return self.M[row * n_cols + col]

    def __setitem__(self, key, value):
        (row, col) = key
        self.M[row * n_cols + col] = value

    def __str__(self):
        return self._format_matrix()

    def _format_matrix(self):
        rows = ''
        for i in range(self.n_rows):
            rows += self._format_row(i)
        return self.graph_fmt % rows

    def _format_row(self, i):
        elems = ''
        for j in range(self.n_cols):
            elems += self._format_elem(i, j)
        return self.row_fmt % elems

    def _format_elem(self, i, j):
        return self.elem_fmt % self.M[i * self.n_cols + j]

# GUI

class Application:
    def on_delete_event(self, widget, event, data = None):
        return False # return False -> GTK will emit destroy signal

    def on_destroy(self, widget, data = None):
        gtk.main_quit()

    def __init__(self):
        window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        window.connect('delete_event', self.on_delete_event)
        window.connect('destroy', self.on_destroy)
        window.set_title('Graph Tool')

        menubar = self._create_menubar()
        workarea = self._create_workarea()

        box = gtk.VBox()

        box.pack_start(menubar, expand = False, fill = False, padding = 0)
        box.pack_start(workarea, expand = True, fill = True, padding = 0)

        window.add(box)

#        sheet = Sheet()
#
#        v1 = sheet.G.add_vertex(10, 10)
#        v2 = sheet.G.add_vertex(20, 20)
#        e1 = sheet.G.add_edge([v1, v2], is_directed = True)
#
#        window.add(sheet.widget)

        window.show_all()

        self.widget = window

    def _create_workarea(self):
        sheet = Sheet()
        notebook = gtk.Notebook()
        notebook.append_page(sheet.widget, gtk.Label(sheet.G.label))
        return notebook

    def _create_menubar(self):
        quit_item = gtk.MenuItem('_Quit')
        quit_item.connect('activate', self.on_menuitem_activate, 'file.quit')
        file_menu = gtk.Menu()
        file_menu.append(quit_item)
        
        about_item = gtk.MenuItem('_About')
        about_item.connect('activate', self.on_menuitem_activate, 'help.about')
        help_menu = gtk.Menu()
        help_menu.append(about_item)

        menu_bar = gtk.MenuBar()
        file_item = gtk.MenuItem('_File')
        file_item.set_submenu(file_menu)
        help_item = gtk.MenuItem('_Help')
        help_item.set_submenu(help_menu)
        menu_bar.append(file_item)
        menu_bar.append(help_item)

        return menu_bar

    def on_menuitem_activate(self, widget, data = None):
        if data == 'file.quit':
            sys.exit(0)

    def main(self):
        gtk.main()

class Sheet:
    def __init__(self):
        self.G = Graph()

        self.vertex_radius = 4

        self.widget = gtk.DrawingArea()
        self.widget.set_size_request(100, 100)
        self.drawable = self.widget.window
        #self.gc = self.drawable.new_gc(foreground = gtk.gdk.Color(0, 0, 0), background = gtk.gdk.Color(65535, 65535, 65535))

        self.widget.connect('expose-event', self.on_expose)

    def on_expose(self, area, event):
        self.draw()

    def draw(self):
        self.style = self.widget.get_style()
        self.gc = self.style.fg_gc[gtk.STATE_NORMAL]
        self.drawable = self.widget.window
        for e in self.G.E:
            if len(e.V) == 2:
                v1 = e.V[0]
                v2 = e.V[1]
                self.drawable.draw_line(self.gc, v1.x, v1.y, v2.x, v2.y)
        for v in self.G.V:
            self.drawable.draw_arc(self.gc, filled = True, x = v.x - self.vertex_radius, y = v.y - self.vertex_radius, width = self.vertex_radius * 2, height = self.vertex_radius * 2, angle1 = 0, angle2 = 360 * 64)

def test():
    G = Graph()
    v1 = G.add_vertex(10, 10)
    v2 = G.add_vertex(20, 20)
    e1 = G.add_edge([v1, v2], is_directed = True)
    M = IncidenceMatrix()
    M.from_graph(G)
    print M

if __name__ == '__main__':
    application = Application()
    application.main()

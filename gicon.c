#include <gtk/gtk.h>

void
on_window_destroy(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}

/*
 * tooltip text
 * tooltip markup
 * has tooltip
 * title
 * name
 * visible
 */

int
main(int argc, char **argv)
{
	GtkWidget *window;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(window), "destroy",
	    G_CALLBACK(on_window_destroy), NULL);
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}

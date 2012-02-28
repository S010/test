#include <gtk/gtk.h>

GtkStatusIcon *status_icon;

void
on_window_destroy(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}

/*
 * name
 * title
 * tooltip text
 * tooltip markup
 * has tooltip
 * visible
 * is embedded
 */

void
on_winid_get_clicked(GtkWidget *widget, gpointer data)
{
	GtkEntry *entry = data;
	char buf[64];
	size_t bufsize = sizeof(buf);
	guint32 id;

	id = gtk_status_icon_get_x11_window_id(status_icon);
	snprintf(buf, bufsize, "%lu", (unsigned long) id);
	gtk_entry_set_text(entry, buf);
}

void
on_name_set_clicked(GtkWidget *widget, gpointer data)
{
	GtkEntry *entry = data;
	const char *name;

	name = gtk_entry_get_text(entry);
	gtk_status_icon_set_name(status_icon, name);
}

void
on_title_get_clicked(GtkWidget *widget, gpointer data)
{
	GtkEntry *entry = data;
	const char *title;

	title = gtk_status_icon_get_title(status_icon);
	if (title != NULL)
		gtk_entry_set_text(entry, title);
}

void
on_title_set_clicked(GtkWidget *widget, gpointer data)
{
	GtkEntry *entry = data;
	const char *title;

	title = gtk_entry_get_text(entry);
	gtk_status_icon_set_title(status_icon, title);
}

void
on_tt_text_get_clicked(GtkWidget *widget, gpointer data)
{
	GtkEntry *entry = data;
	const char *text;

	text = gtk_status_icon_get_tooltip_text(status_icon);
	if (text != NULL)
		gtk_entry_set_text(entry, text);
}

void
on_tt_text_set_clicked(GtkWidget *widget, gpointer data)
{
	GtkEntry *entry = data;
	const char *text;

	text = gtk_entry_get_text(entry);
	if (text != NULL && strcmp(text, "") != 0)
		gtk_status_icon_set_tooltip_text(status_icon, text);
}

void
on_tt_markup_get_clicked(GtkWidget *widget, gpointer data)
{
	GtkEntry *entry = data;
	const char *markup;

	markup = gtk_status_icon_get_tooltip_markup(status_icon);
	if (markup != NULL)
		gtk_entry_set_text(entry, markup);
}

void
on_tt_markup_set_clicked(GtkWidget *widget, gpointer data)
{
	GtkEntry *entry = data;
	const char *markup;
	
	markup = gtk_entry_get_text(entry);
	if (*markup != '\0')
		gtk_status_icon_set_tooltip_markup(status_icon, markup);
}

void
on_has_tooltip_get_clicked(GtkWidget *widget, gpointer data)
{
	GtkEntry *entry = data;
	gboolean val;

	val = gtk_status_icon_get_has_tooltip(status_icon);
	if (val)
		gtk_entry_set_text(entry, "true");
	else
		gtk_entry_set_text(entry, "false");
}

void
on_has_tooltip_set_clicked(GtkWidget *widget, gpointer data)
{
	GtkEntry *entry = data;
	const char *text;
	gboolean val;

	text = gtk_entry_get_text(entry);
	if (!strcmp(text, "true"))
		gtk_status_icon_set_has_tooltip(status_icon, TRUE);
	else
		gtk_status_icon_set_has_tooltip(status_icon, FALSE);
}

void
on_visible_get_clicked(GtkWidget *widget, gpointer data)
{
	GtkEntry *entry = data;
	gboolean val;

	val = gtk_status_icon_get_visible(status_icon);
	if (val)
		gtk_entry_set_text(entry, "true");
	else
		gtk_entry_set_text(entry, "false");
}

void
on_visible_set_clicked(GtkWidget *widget, gpointer data)
{
	GtkEntry *entry = data;
	const char *text;
	gboolean val;

	text = gtk_entry_get_text(entry);
	if (!strcmp(text, "true"))
		gtk_status_icon_set_visible(status_icon, TRUE);
	else
		gtk_status_icon_set_visible(status_icon, FALSE);
}

void
on_isembed_get_clicked(GtkWidget *widget, gpointer data)
{
	GtkEntry *entry = data;
	gboolean val;

	val = gtk_status_icon_is_embedded(status_icon);
	if (val)
		gtk_entry_set_text(entry, "true");
	else
		gtk_entry_set_text(entry, "false");
}


GtkWidget *
add_text_field(GtkWidget *box, const char *label)
{
	GtkWidget *hbox, *lbl, *txt;

	hbox = gtk_hbox_new(FALSE, 0);
	lbl = gtk_label_new(label);
	txt = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), lbl, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), txt, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(box),  hbox, FALSE, FALSE, 0);

	return txt;
}

void
add_getset_buttons(GtkWidget *box, GtkWidget *txt, void *get_cb, void *set_cb)
{
	GtkWidget *hbox, *get, *set;

	hbox = gtk_hbox_new(FALSE, 0);

	if (get_cb != NULL) {
		get = gtk_button_new_with_label("Get");
		g_signal_connect(G_OBJECT(get), "clicked",
		    G_CALLBACK(get_cb), txt);
		gtk_box_pack_end(GTK_BOX(hbox), get, FALSE, FALSE, 0);
	}
	if (set_cb != NULL) {
		set = gtk_button_new_with_label("Set");
		g_signal_connect(G_OBJECT(set), "clicked",
		    G_CALLBACK(set_cb), txt);
		gtk_box_pack_end(GTK_BOX(hbox), set, FALSE, FALSE, 0);
	}

	gtk_box_pack_start(GTK_BOX(box),  hbox, FALSE, FALSE, 0);
}

void
add_id_label(GtkWidget *box, GtkStatusIcon *status_icon)
{
}

int
main(int argc, char **argv)
{
	GtkWidget *window, *main_vbox, *hbox, *lbl, *txt, *get, *set;
	uint id;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(window), "destroy",
	    G_CALLBACK(on_window_destroy), NULL);
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

	main_vbox = gtk_vbox_new(FALSE, 0);

	status_icon = gtk_status_icon_new_from_stock(GTK_STOCK_OPEN);

	add_id_label(main_vbox, status_icon);

	/* Window id */

	txt = add_text_field(main_vbox, "Window id");
	add_getset_buttons(main_vbox, txt, on_winid_get_clicked, NULL);

	/* Name */

	txt = add_text_field(main_vbox, "Name");
	add_getset_buttons(main_vbox, txt, NULL,
	    on_name_set_clicked);

	/* Title */

	txt = add_text_field(main_vbox, "Title");
	add_getset_buttons(main_vbox, txt, on_title_get_clicked,
	    on_title_set_clicked);

	/* Tooltip text */

	txt = add_text_field(main_vbox, "Tooltip text");
	add_getset_buttons(main_vbox, txt, on_tt_text_get_clicked,
	    on_tt_text_set_clicked);

	/* Tooltip markup */

	txt = add_text_field(main_vbox, "Tooltip markup");
	add_getset_buttons(main_vbox, txt, on_tt_markup_get_clicked,
	    on_tt_markup_set_clicked);

	/* Has tooltip? */

	txt = add_text_field(main_vbox, "Has tooltip?");
	add_getset_buttons(main_vbox, txt, on_has_tooltip_get_clicked,
	    on_has_tooltip_get_clicked);

	/* Visible */

	txt = add_text_field(main_vbox, "Visible");
	add_getset_buttons(main_vbox, txt, on_visible_get_clicked,
	    on_visible_set_clicked);

	/* Is embedded */

	txt = add_text_field(main_vbox, "Is embedded");
	add_getset_buttons(main_vbox, txt, on_isembed_get_clicked, NULL);


	gtk_container_add(GTK_CONTAINER(window), main_vbox);
	gtk_widget_show_all(window);



	gtk_main();

	return 0;
}

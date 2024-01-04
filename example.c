#ifndef __GTK_H__
#include <gtk/gtk.h>
#endif
// #include <glib/gprintf.h>
#ifndef _xc_chat_view_h_
#include "xcchatview.h"
#endif

static void
create_menu_item (GMenu *menu,
		const gchar *const label, const gchar *const action,
		const gchar *const icon,  const gchar *const accel) {
	GMenuItem *item = g_menu_item_new (label, action);

	if (icon)
		g_menu_item_set_attribute (item, G_MENU_ATTRIBUTE_ICON, "s", icon, NULL);
	if (accel)
		g_menu_item_set_attribute (item, "accel", "s", accel, NULL);

	g_menu_append_item (menu, item);
	g_object_unref (item);
}

static void
cb_quit (GSimpleAction *simple,
	GVariant *parameter,
	gpointer app)
{
	g_application_quit(G_APPLICATION(app));
}

static void
example_destroy (GtkWidget *win, gpointer app) {
	g_application_quit(G_APPLICATION(app));
}

static void
example_started (GtkApplication *app, gpointer user_data) {
	GMenu *menu = g_menu_new ();

	if (gtk_application_prefers_app_menu (app))
		;
	else
		gtk_application_set_app_menu (app, G_MENU_MODEL(menu));

	static const GActionEntry acts[] = {
		{"quit", cb_quit, NULL, NULL, NULL},
	};
	g_action_map_add_action_entries (G_ACTION_MAP (app), acts, G_N_ELEMENTS (acts), app);
	create_menu_item (menu, "_Quit", "app.quit", "application-exit", "<CTRL>Q");

	g_object_unref (menu);
}

static void
example_activated (GtkApplication *app, gpointer user_data) {
	GtkWidget *win = gtk_application_window_new (app);
	gtk_window_set_title (GTK_WINDOW (win), "Example");
	g_signal_connect (win, "destroy", G_CALLBACK (example_destroy), app);

	GtkWidget *mbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_add (GTK_CONTAINER (win), mbox);
	GtkWidget *stack = gtk_stack_new ();
	gtk_box_pack_start (GTK_BOX (mbox), stack, TRUE, TRUE, 0);

	GtkWidget *sw1 = gtk_scrolled_window_new (NULL, NULL);
	XcChatView *xccv1 = xc_chat_view_new ();
	gtk_stack_add_named (GTK_STACK (stack), sw1, "One");
	gtk_container_add (GTK_CONTAINER (sw1), GTK_WIDGET (xccv1->tview));
	
	GtkWidget *sw2 = gtk_scrolled_window_new (NULL, NULL);
	XcChatView *xccv2 = (xc_chat_view_new ());
	gtk_stack_add_named (GTK_STACK (stack), sw2, "Two");
	gtk_container_add (GTK_CONTAINER (sw2), GTK_WIDGET (xccv2->tview));

	gtk_widget_show_all (win);
}


int
main (int argc, char *argv[]) {
	int r;
	GtkApplication *app = gtk_application_new ("com.github.example", G_APPLICATION_DEFAULT_FLAGS); // G_APPLICATION_FLAGS_NONE);

	g_signal_connect (app, "startup",  G_CALLBACK (example_started),   NULL);
	g_signal_connect (app, "activate", G_CALLBACK (example_activated), NULL);

	r = g_application_run (G_APPLICATION (app), argc, argv);
	g_object_unref (app);
	return r;
}


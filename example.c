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
//	if (target)
//		g_menu_item_set_attribute (item, G_MENU_ATTRIBUTE_TARGET, "s", target, NULL);

	g_menu_append_item (menu, item);
	g_object_unref (item);
}

//state part 10
//static void cb_state (GSimpleAction *action,
//	GVariant *state,
//	gpointer app) {
//	g_simple_action_set_state (action, state);
//}

static void
cb_pgup (GSimpleAction *simple, GVariant *parameter, gpointer stack) {
	GtkWidget *ch = gtk_stack_get_child_by_name (GTK_STACK (stack), "One");

	gtk_stack_set_visible_child (GTK_STACK (stack), ch);
}

static void
cb_pgdn (GSimpleAction *simple, GVariant *parameter, gpointer stack) {
	GtkWidget *ch = gtk_stack_get_child_by_name (GTK_STACK (stack), "Two");

	gtk_stack_set_visible_child (GTK_STACK (stack), ch);
}

static void
cb_quit (GSimpleAction *simple, GVariant *parameter, gpointer app) {
	g_application_quit (G_APPLICATION (app));
}

static void
example_destroy (GtkWidget *win, gpointer app) {
	g_application_quit (G_APPLICATION (app));
}


static void
create_tabs (XcChatView *xccv, GtkWidget *stack, char *name) {
	GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (sw), GTK_WIDGET (xccv->tview));
	gtk_stack_add_named (GTK_STACK (stack), sw, name);
}

/* static void
 example_started (GtkApplication *app, gpointer user_data) {
	if (gtk_application_prefers_app_menu (app))
		gtk_application_set_app_menu (app, G_MENU_MODEL(menu));
} */

static void
example_activated (GtkApplication *app, gpointer user_data) {
	GtkWidget *win = gtk_application_window_new (app);
	gtk_window_set_title (GTK_WINDOW (win), "Example");
	g_signal_connect (win, "destroy", G_CALLBACK (example_destroy), app);

	GtkWidget *mbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add (GTK_CONTAINER (win), mbox);

	GMenu *mmenu = g_menu_new ();
	GMenu *smenu = g_menu_new ();
	g_menu_append_submenu (mmenu, "App", G_MENU_MODEL (smenu));
	gtk_application_set_menubar (app, G_MENU_MODEL (mmenu));
	g_object_unref (mmenu);
	g_object_unref (smenu);

	GtkWidget *stack = gtk_stack_new ();
	gtk_box_pack_end   (GTK_BOX (mbox), stack,  TRUE,  TRUE, 0);

	const GActionEntry acts[] = {
		{"quit", cb_quit, NULL, NULL, NULL},
	};
	const GActionEntry sacts[] = {
		{"pgup", cb_pgup, NULL, NULL, NULL},
		{"pgdn", cb_pgdn, NULL, NULL, NULL},
	};
	g_action_map_add_action_entries (G_ACTION_MAP (app),  acts, G_N_ELEMENTS  (acts), app);
	g_action_map_add_action_entries (G_ACTION_MAP (app), sacts, G_N_ELEMENTS (sacts), stack);

	create_menu_item (smenu, "Page _Up",   "app.pgup", NULL, "<CTRL>Prior");
	create_menu_item (smenu, "Page _Down", "app.pgdn", NULL, "<CTRL>Next");
	create_menu_item (smenu, "_Quit", "app.quit", "application-exit", "<CTRL>Q");

	XcChatView *xccv1 = (xc_chat_view_new ());
	XcChatView *xccv2 = (xc_chat_view_new ());
	create_tabs (xccv1, stack, "One");
	create_tabs (xccv2, stack, "Two");

	gtk_widget_show_all (win);

/*
	xc_chat_view_set_scrollback_file (xccv1, "foo/rrr");
*/
	xc_chat_view_set_scrollback_file (xccv1, "text1");
	xc_chat_view_set_scrollback_file (xccv2, "text2");
}


int
main (int argc, char *argv[]) {
	int r;
	GtkApplication *app = gtk_application_new ("com.github.example", G_APPLICATION_DEFAULT_FLAGS); // G_APPLICATION_FLAGS_NONE);

//	g_signal_connect (app, "startup",  G_CALLBACK (example_started),   NULL);
	g_signal_connect (app, "activate", G_CALLBACK (example_activated), NULL);

	r = g_application_run (G_APPLICATION (app), argc, argv);
	g_object_unref (app);
	return r;
}


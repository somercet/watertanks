#ifndef __GTK_H__
#include <gtk/gtk.h>
#endif
// #include <glib/gprintf.h>
#ifndef _xc_chat_view_h_
#include "xcchatview.h"
#endif

GSettings *
settings;
static GList *
stakk = NULL;

struct
Xccvbit {
	gpointer xccv;
	gpointer child;
};

static gboolean
searchflags[3];
static gpointer
searchbits[8];

enum
isrch {
	SI_REGEX,
	SI_CASE,
	SI_ALL,
	SI_LABEL,
	SI_ENTRY,
	SI_DOWN,
	SI_UP,
	SI_BAR
};

static void
create_menu_item (GMenu *menu,
		const gchar *const label, const gchar *const action,
		const gchar *const icon,  const gchar *const accel,
		const gchar *const target) {
	GMenuItem *item = g_menu_item_new (label, action);

	if (icon)
		g_menu_item_set_attribute (item, G_MENU_ATTRIBUTE_ICON, "s", icon, NULL);
	if (accel)
		g_menu_item_set_attribute (item, "accel", "s", accel, NULL);
	if (target)
		g_menu_item_set_attribute (item, G_MENU_ATTRIBUTE_TARGET, "s", target, NULL);

	g_menu_append_item (menu, item);
	g_object_unref (item);
}

/*
state part 10
static void cb_state (GSimpleAction *action,
	GVariant *state,
	gpointer app) {
	g_simple_action_set_state (action, state);
}
*/

static XcChatView *
get_active_xccv (GtkStack *stack) {
	struct Xccvbit *tab;
	GtkWidget *current = gtk_stack_get_visible_child (stack);

	GList *l;
	for (l = stakk; l != NULL; l = l->next) {
		tab = l->data;
		if (current == tab->child)
			break;
	}
	return tab->xccv;
}


static void
cb_copy (GSimpleAction *simple, GVariant *parameter, gpointer stck) {
	XcChatView *xccv = get_active_xccv (GTK_STACK (stck));
	xc_chat_view_copy_selection (xccv);
}


static void
example_destroy (GtkWidget *win, gpointer app) {
	g_settings_sync ();
	g_object_unref (settings);
	g_application_quit (G_APPLICATION (app));
}


static void
cb_quit (GSimpleAction *simple, GVariant *parameter, gpointer win) {
	GtkApplication *app = gtk_window_get_application (win);
	example_destroy (win, app);
}


static void
chpg (GtkStack *stack, gboolean up) {
	struct Xccvbit *tab;
	gboolean got = FALSE;
	GtkWidget *current = gtk_stack_get_visible_child (stack);

	GList *l;
	for (l = stakk; l != NULL; ) {
		tab = l->data;
		if (got) {
			gtk_stack_set_visible_child (stack, tab->child);
			break;
		}
		if (current == tab->child) {
			got = TRUE;
			if (up) {
				l = l->prev;
				continue;
			}
		}
		l = l->next;
	}
}


static void
cb_pgup (GSimpleAction *simple, GVariant *parameter, gpointer stck) {
	chpg (GTK_STACK (stck), TRUE);
}
static void
cb_pgdn (GSimpleAction *simple, GVariant *parameter, gpointer stck) {
	chpg (GTK_STACK (stck), FALSE);
}


static void
create_tabs (XcChatView *xccv, GtkWidget *stack, char *name) {
	GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (sw), GTK_WIDGET (xccv->tview));
	gtk_stack_add_titled (GTK_STACK (stack), sw, name, name);

	struct Xccvbit *newtab = g_new (struct Xccvbit, 1);
	newtab->xccv = xccv;
	newtab->child = sw;
	stakk = g_list_append (stakk, newtab);
}


static void
cb_find (GSimpleAction *simple, GVariant *parameter, gpointer stck) {
	gboolean tog = TRUE;

	if (gtk_search_bar_get_search_mode (searchbits[SI_BAR]))
		tog = FALSE;
	gtk_search_bar_set_search_mode (searchbits[SI_BAR], tog);
}

static void
run_search (GtkSearchEntry *entry, gpointer stck) {
	XcChatView *xccv = get_active_xccv (GTK_STACK (stck));
	xc_chat_view_run_search (xccv, gtk_entry_get_text (GTK_ENTRY (entry)));
}

static void
cb_toggled (GtkToggleButton *togged, gpointer stack) {
	gboolean tmp = gtk_toggle_button_get_active (searchbits[SI_REGEX]);
	g_settings_set_boolean (settings, "text-search-regexp", tmp);
	searchflags[SI_REGEX] = tmp;
}

static void
create_searchbar (GtkWidget *bar, GtkWidget *stack) {
	GtkWidget *bx = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	GtkBox *box = GTK_BOX (bx);
	searchbits[SI_BAR]   = bar;
	searchbits[SI_UP]    = gtk_button_new_from_icon_name ("go-up", GTK_ICON_SIZE_SMALL_TOOLBAR);
	searchbits[SI_DOWN]  = gtk_button_new_from_icon_name ("go-down", GTK_ICON_SIZE_SMALL_TOOLBAR);
	searchbits[SI_ENTRY] = gtk_search_entry_new ();
	searchbits[SI_ALL]   = gtk_toggle_button_new_with_label ("All");
	searchbits[SI_CASE]  = gtk_toggle_button_new_with_label ("a=A"); // a≠A
	searchbits[SI_REGEX] = gtk_toggle_button_new_with_label (".*");

	gtk_container_add (GTK_CONTAINER (bar), bx);
	gtk_search_bar_connect_entry (GTK_SEARCH_BAR (bar), GTK_ENTRY (searchbits[SI_ENTRY]));
	gtk_search_bar_set_show_close_button (GTK_SEARCH_BAR (bar), TRUE);
	gtk_box_pack_start (box, searchbits[SI_UP],    FALSE, FALSE, 0);
	gtk_box_pack_start (box, searchbits[SI_DOWN],  FALSE, FALSE, 0);
	gtk_box_pack_start (box, searchbits[SI_ENTRY],  TRUE,  TRUE, 0);
	gtk_box_pack_start (box, searchbits[SI_ALL],   FALSE, FALSE, 0);
	gtk_box_pack_start (box, searchbits[SI_CASE],  FALSE, FALSE, 0);
	gtk_box_pack_start (box, searchbits[SI_REGEX], FALSE, FALSE, 0);

	g_signal_connect (searchbits[SI_ENTRY], "search-changed", G_CALLBACK (run_search), stack);
	g_signal_connect (searchbits[SI_REGEX], "toggled",        G_CALLBACK (cb_toggled), stack);
	gboolean tmp = g_settings_get_boolean (settings, "text-search-regexp");
	gtk_toggle_button_set_active (searchbits[SI_REGEX], tmp);
	searchflags[SI_REGEX] = tmp;



/*
searchflags[4];

// stamp-text
// stamp-text-format
// text-search-regexp

gtk_label_set_selectable (, false)
gtk_label_new (NULL);
gtk_label_set_width_chars (, 9) [10 of 102]
gtk_label_set_line_wrap FALSE
stack runs the search entry and label, and a timer to update the label.
xccv runs the search and updates the search info {
guint search_item
guint total_found
bool search_running 
char search_flags }

global pointer array for stack to use searchbar

up/down tell stack, and stack calls function that Ctrl-G does.
ignore All for now
cse and rgx set flags, flags are in XcChatView

xccv needs signal for bar to signal updates. or semaphore? Asynch queue?
Or just int func return?

*/
}

static void
cb_wrap (GSimpleAction *simple, GVariant *parameter, gpointer stck) {
	XcChatView *xccv = get_active_xccv (GTK_STACK (stck));
	xc_chat_view_set_wordwrap (xccv, TRUE);
}

/* static void
example_started (GtkApplication *app, gpointer user_data) {
	if (gtk_application_prefers_app_menu (app))
		gtk_application_set_app_menu (app, G_MENU_MODEL(menu));
} */

static void
example_activated (GtkApplication *app, gpointer user_data) {
	GtkWidget *win = gtk_application_window_new (app);
	//gtk_window_set_application (GTK_WINDOW (win), app);
	gtk_window_set_title (GTK_WINDOW (win), "Example");
	gtk_window_set_default_size (GTK_WINDOW (win), 400, 300);
	g_signal_connect (win, "destroy", G_CALLBACK (example_destroy), app);

	GtkWidget *mbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add (GTK_CONTAINER (win), mbox);

	settings = g_settings_new("com.github.example");

	GMenu *mmenu = g_menu_new ();
	GMenu *smenu = g_menu_new ();
	create_menu_item (smenu, "_Copy",	"app.copy",	NULL,	"<ctrl>c",	NULL);
	create_menu_item (smenu, "_Word Wrap",	"app.wrap",	NULL,	"<ctrl><shift>w", "wrapped");
	create_menu_item (smenu, "_Find",	"app.find",	NULL,	"<ctrl>f",	NULL);
	create_menu_item (smenu, "Page _Up",	"app.pgup",	NULL,	"<ctrl>Prior",	NULL);
	create_menu_item (smenu, "Page _Down",	"app.pgdn",	NULL,	"<ctrl>Next",	NULL);
	create_menu_item (smenu, "_Quit",	"app.quit",	"application-exit",
									"<ctrl>q",	NULL);
	g_menu_append_submenu (mmenu, "E_xample", G_MENU_MODEL (smenu));
	gtk_application_set_menubar (app, G_MENU_MODEL (mmenu));
	g_object_unref (mmenu);
	g_object_unref (smenu);

	GtkWidget *stack = gtk_stack_new ();
	GtkWidget *sbar = gtk_search_bar_new ();
	create_searchbar (sbar, stack);
	gtk_box_pack_start (GTK_BOX (mbox), stack,  TRUE,  TRUE, 0);
	gtk_box_pack_start (GTK_BOX (mbox),  sbar, FALSE, FALSE, 0);

	const GActionEntry wacts[] = {
		{"quit", cb_quit, NULL, NULL, NULL, {0, 0, 0}},
	};
	const GActionEntry sacts[] = {
		{"copy", cb_copy, NULL, NULL, NULL, {0, 0, 0}},
		{"wrap", cb_wrap, NULL, NULL, NULL, {0, 0, 0}},
		{"pgup", cb_pgup, NULL, NULL, NULL, {0, 0, 0}},
		{"pgdn", cb_pgdn, NULL, NULL, NULL, {0, 0, 0}},
		{"find", cb_find, NULL, NULL, NULL, {0, 0, 0}},
	};

	g_action_map_add_action_entries (G_ACTION_MAP (app), wacts, G_N_ELEMENTS (wacts), win);
	g_action_map_add_action_entries (G_ACTION_MAP (app), sacts, G_N_ELEMENTS (sacts), stack);

	XcChatView *xccv1 = (xc_chat_view_new ());
	XcChatView *xccv2 = (xc_chat_view_new ());
	create_tabs (xccv1, stack, "One");
	create_tabs (xccv2, stack, "Two");

	gtk_widget_show_all (win);
/*
	xc_chat_view_set_scrollback_file (xccv3, "rrr");
	xc_chat_view_set_scrollback_file (xccv3, "foo/rrr");
	xc_chat_view_set_background (xccv1, "/home/peter/Pictures/tile_5020.png");
	xc_chat_view_set_font (xccv1, "Arimo 12");
*/
	xc_chat_view_set_scrollback_file (xccv1, "text1");
	xc_chat_view_set_scrollback_file (xccv2, "text2");
}


int
main (int argc, char *argv[]) {
	int r;
	GtkApplication *app = gtk_application_new ("com.github.example", G_APPLICATION_DEFAULT_FLAGS); // G_APPLICATION_FLAGS_NONE);

	//g_signal_connect (app, "startup",  G_CALLBACK (example_started),   NULL);
	g_signal_connect (app, "activate", G_CALLBACK (example_activated), NULL);

	r = g_application_run (G_APPLICATION (app), argc, argv);
	g_object_unref (app);
	g_clear_list (&stakk, (GDestroyNotify) g_free);
	return r;
}


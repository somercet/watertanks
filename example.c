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

static XcChatView *
Lastlog = NULL;

xc_search_flags
search_flags = 0;

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
	SI_NEXT,
	SI_PREV,
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
	XcChatView *xccvold;//, *xccvnew;
	struct Xccvbit *tab;
	gboolean got = FALSE,
		sb_visible = gtk_search_bar_get_search_mode (searchbits[SI_BAR]);
	GtkWidget *current = gtk_stack_get_visible_child (stack);

	if (sb_visible)
		xccvold = get_active_xccv (GTK_STACK (stack));

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

	if (sb_visible && (xccvold != tab->xccv)) {
		gtk_widget_show (XC_CHAT_VIEW (tab->xccv)->search_widget);
		gtk_widget_hide (xccvold->search_widget);
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
	gtk_stack_add_titled (GTK_STACK (stack), sw, name, name);
	gtk_container_add (GTK_CONTAINER (sw), GTK_WIDGET (xccv->tview));

	struct Xccvbit *newtab = g_new (struct Xccvbit, 1);
	newtab->xccv = xccv;
	newtab->child = sw;
	stakk = g_list_append (stakk, newtab);

	gtk_box_pack_start (searchbits[SI_LABEL], xccv->search_widget, FALSE, FALSE, 0);
}


static void
cb_find (GSimpleAction *simple, GVariant *parameter, gpointer stack) {
	gboolean visible = ! gtk_search_bar_get_search_mode (searchbits[SI_BAR]);

	gtk_search_bar_set_search_mode (searchbits[SI_BAR], visible);

	XcChatView *xccv = get_active_xccv (GTK_STACK (stack));
	if (visible)
		gtk_widget_show (xccv->search_widget);
	else
		gtk_widget_hide (xccv->search_widget);
// gtk_entry_set_icon_from_icon_name (searchbits[SI_ENTRY], GTK_ENTRY_ICON_SECONDARY, "dialog-error");
}

static void
run_search (GtkSearchEntry *entry, gpointer stack) {
	XcChatView *xccv = get_active_xccv (GTK_STACK (stack));
	xc_chat_view_run_search (xccv, gtk_entry_get_text (GTK_ENTRY (entry)), search_flags);
//gtk_xtext_search (GTK_XTEXT (sess->gui->xtext), text, flags, &err);
}

static void
next_search (gpointer *stack, gboolean direction) {
	XcChatView *xccv = get_active_xccv (GTK_STACK (stack));
	xc_chat_view_next_search (xccv, direction);
}

static void
cb_next (GSimpleAction *simple, GVariant *parameter, gpointer stack) {
	next_search (stack, TRUE);
}

static void
cb_bnext (GtkButton *togged, gpointer stack) {
	next_search (stack, TRUE);
}

static void
cb_prev (GSimpleAction *simple, GVariant *parameter, gpointer stack) {
	next_search (stack, FALSE);
}

static void
cb_bprev (GtkButton *togged, gpointer stack) {
	next_search (stack, FALSE);
}

static void
cb_toggled (GtkToggleButton *togged, gpointer stack) {
	short c;

	for (c = 0; c < 3 ; c++)
		if (togged == searchbits[c])
			break;
	if (c == 3) { // just in case...
		g_printerr ("Error processing search flags: line %d, %s().\n", __LINE__, __FILE__);
		return;
	}

	searchflags[c] = gtk_toggle_button_get_active (searchbits[c]);

	if (c == SI_CASE) {
		if (searchflags[SI_CASE])
			gtk_button_set_label (GTK_BUTTON (togged), "a≠A");
		else
			gtk_button_set_label (GTK_BUTTON (togged), "a=A");
	}

	search_flags =	( searchflags[0] ? regexp     : 0 ) |
			( searchflags[1] ? case_match : 0 ) |
			( searchflags[2] ? highlight  : 0 ) ;

	if (c != SI_ALL) // no highlight for now
		run_search (searchbits[SI_ENTRY], stack);
}

static void
cb_lastlog (GtkButton *togged, gpointer stack) {
	XcChatView *xccv = get_active_xccv (GTK_STACK (stack));

	if (Lastlog == NULL) {
		Lastlog = xc_chat_view_new ();
		create_tabs (Lastlog, GTK_WIDGET (stack), "Lastlog");
		gtk_widget_show_all (stack);
	}
	xc_chat_view_lastlog (xccv, gtk_entry_get_text (searchbits[SI_ENTRY]), Lastlog);
}

static void
create_searchbar (GtkWidget *bar, GtkWidget *stack) {
	GtkWidget *bx = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	GtkBox *box = GTK_BOX (bx);
	searchbits[SI_BAR]   = bar;
	searchbits[SI_PREV]  = gtk_button_new_from_icon_name ("go-previous", GTK_ICON_SIZE_SMALL_TOOLBAR);
	searchbits[SI_NEXT]  = gtk_button_new_from_icon_name ("go-next", GTK_ICON_SIZE_SMALL_TOOLBAR);
	searchbits[SI_ENTRY] = gtk_search_entry_new ();
	searchbits[SI_ALL]   = gtk_toggle_button_new_with_label ("All");
	searchbits[SI_CASE]  = gtk_toggle_button_new_with_label ("a=A"); // a≠A
	searchbits[SI_REGEX] = gtk_toggle_button_new_with_label (".*");

	GtkWidget *lastlog = gtk_button_new_with_label ("Lastlog");

	gtk_container_add (GTK_CONTAINER (bar), bx);
	gtk_search_bar_connect_entry (GTK_SEARCH_BAR (bar), GTK_ENTRY (searchbits[SI_ENTRY]));
	gtk_search_bar_set_show_close_button (GTK_SEARCH_BAR (bar), TRUE);

	gtk_box_pack_start (box, lastlog, FALSE, FALSE, 0);

	searchbits[SI_LABEL] = (GtkBox *) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

	gtk_box_pack_start (box, searchbits[SI_PREV],	FALSE, FALSE, 0);
	gtk_box_pack_start (box, searchbits[SI_NEXT],	FALSE, FALSE, 0);
	gtk_box_pack_start (box, searchbits[SI_ENTRY],	TRUE,  TRUE,  0);
	gtk_box_pack_start (box, searchbits[SI_LABEL],	FALSE, FALSE, 2);
	gtk_box_pack_start (box, searchbits[SI_ALL],	FALSE, FALSE, 0);
	gtk_box_pack_start (box, searchbits[SI_CASE],	FALSE, FALSE, 0);
	gtk_box_pack_start (box, searchbits[SI_REGEX],	FALSE, FALSE, 0);

	g_settings_bind (settings, "text-search-highlight-all",	searchbits[SI_ALL],	"active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (settings, "text-search-case-match",	searchbits[SI_CASE],	"active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (settings, "text-search-regexp",	searchbits[SI_REGEX],	"active", G_SETTINGS_BIND_DEFAULT);
	g_signal_connect (searchbits[SI_ENTRY],	"search-changed", G_CALLBACK (run_search), stack);
	g_signal_connect (searchbits[SI_ENTRY],	"activate",	G_CALLBACK (run_search), stack);
	g_signal_connect (searchbits[SI_ALL],	"toggled",	G_CALLBACK (cb_toggled), stack);
	g_signal_connect (searchbits[SI_CASE],	"toggled",	G_CALLBACK (cb_toggled), stack);
	g_signal_connect (searchbits[SI_REGEX],	"toggled",	G_CALLBACK (cb_toggled), stack);
	g_signal_connect (searchbits[SI_PREV],	"clicked",	G_CALLBACK (cb_bprev), stack);
	g_signal_connect (searchbits[SI_NEXT],	"clicked",	G_CALLBACK (cb_bnext), stack);

	g_signal_connect (lastlog, "clicked", G_CALLBACK (cb_lastlog), stack);
}
	//cb_toggled (searchbits[SI_REGEX], stack);

/*

activate-current-link
Applications may also emit the signal with g_signal_emit_by_name()
if they need to control activation of URIs programmatically.

searchflags[4];

// stamp-text
// stamp-text-format

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

static void
cb_time (GSimpleAction *simple, GVariant *parameter, gpointer stack) {
	XcChatView *xccv = get_active_xccv (GTK_STACK (stack));
	GVariant *state = g_action_get_state (G_ACTION (simple));
	gboolean flag = g_variant_get_boolean (state);
	g_variant_unref (state);

	g_simple_action_set_state (simple, g_variant_new_boolean (!flag));
	xc_chat_view_set_time_stamp (xccv, !flag);
}

static void
cb_wrap (GSimpleAction *simple, GVariant *parameter, gpointer stack) {
	XcChatView *xccv = get_active_xccv (GTK_STACK (stack));
	GVariant *state = g_action_get_state (G_ACTION (simple));
	gboolean flag = g_variant_get_boolean (state);
	g_variant_unref (state);

	g_simple_action_set_state (simple, g_variant_new_boolean (!flag));
	xc_chat_view_set_wordwrap (xccv, !flag);
}

/* static void
example_started (GtkApplication *app, gpointer user_data) {
	if (gtk_application_prefers_app_menu (app))
		gtk_application_set_app_menu (app, G_MENU_MODEL (menu));
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

	settings = g_settings_new ("com.github.example");

	GMenu *mmenu = g_menu_new ();
	GMenu *smenu = g_menu_new ();
	create_menu_item (smenu, "_Copy",	"app.copy",	NULL,	"<ctrl>c",	NULL);
	create_menu_item (smenu, "_Word Wrap",	"app.wrap",	NULL,	"<ctrl><shift>w", NULL);
	create_menu_item (smenu, "Show _Times",	"app.time",	NULL,	"<ctrl>t",	NULL);
	create_menu_item (smenu, "_Find",	"app.find",	NULL,	"<ctrl>f",	NULL);
	create_menu_item (smenu, "Find _Next",	"app.next",	NULL,	"<ctrl>g",	NULL);
	create_menu_item (smenu, "Find _Previous",	"app.prev",	NULL,	"<ctrl><shift>g",	NULL);
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
		{"copy", cb_copy, NULL,    NULL, NULL, {0, 0, 0}},
		{"wrap", cb_wrap, NULL, "false", NULL, {0, 0, 0}},
		{"time", cb_time, NULL,  "true", NULL, {0, 0, 0}},
		{"pgup", cb_pgup, NULL,    NULL, NULL, {0, 0, 0}},
		{"pgdn", cb_pgdn, NULL,    NULL, NULL, {0, 0, 0}},
		{"next", cb_next, NULL,    NULL, NULL, {0, 0, 0}},
		{"prev", cb_prev, NULL,    NULL, NULL, {0, 0, 0}},
		{"find", cb_find, NULL,    NULL, NULL, {0, 0, 0}},
	};

	g_action_map_add_action_entries (G_ACTION_MAP (app), wacts, G_N_ELEMENTS (wacts), win);
	g_action_map_add_action_entries (G_ACTION_MAP (app), sacts, G_N_ELEMENTS (sacts), stack);

	GtkCssProvider *cssProvider = gtk_css_provider_new();
	gtk_css_provider_load_from_path(cssProvider, "theme.css", NULL);
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
		GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_USER);

	gtk_widget_show_all (win);

	XcChatView *xccv1 = (xc_chat_view_new ());
	XcChatView *xccv2 = (xc_chat_view_new ());
	XcChatView *xccv3 = (xc_chat_view_new ());
	create_tabs (xccv1, stack, "One");
	create_tabs (xccv2, stack, "Two");
	create_tabs (xccv3, stack, "Three");
	gtk_widget_show_all (stack);
/*
	xc_chat_view_set_scrollback_file (xccv3, "rrr");
	xc_chat_view_set_scrollback_file (xccv3, "foo/rrr");
	xc_chat_view_set_background (xccv1, "/home/peter/Pictures/tile_5020.png");
	xc_chat_view_set_font (xccv1, "Arimo 12");
*/
	xc_chat_view_set_scrollback_file (xccv1, "text1");
	xc_chat_view_set_scrollback_file (xccv2, "text2");
	xc_chat_view_set_scrollback_file (xccv3, "text3");
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

/*
g_print ("test %d: %s\n", __LINE__, __FILE__);
__func__
*/

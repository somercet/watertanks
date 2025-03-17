#ifndef __GTK_H__
#include <gtk/gtk.h>
#endif
// #include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#ifndef _xc_search_flags_h_
#include "xc_search_flags.h"
#endif
#ifndef _xc_chat_view_h_
#include "xcchatview.h"
#endif

static GList *
stakk = NULL;

struct
tabbit {
	XcChatView *xccv;
	GtkWidget *gen;
//	gpointer xccv;
//	gpointer gen;
};

static struct atview
Atv;

static XcChatView *
Lastlog = NULL;

gint
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

static struct tabbit *
get_active_tab (GtkStack *stack) {
	struct tabbit *tab;
	GtkWidget *current = gtk_stack_get_visible_child (stack);

	GList *l;
	for (l = stakk; l != NULL; l = l->next) {
		tab = l->data;
		if ((tab->xccv && tab->xccv->atv) || current == tab->gen)
			break;
	}
	return tab;
}
*/

// if (gen == NULL) xccv active + attached else gen active & xccv attached
static struct tabbit *
get_actives (GtkStack *stack) {
	struct tabbit *tab = NULL, *answer = g_new0 (struct tabbit, 1);
	GtkWidget *current = gtk_stack_get_visible_child (stack);
	short left = 2;

//	for (GList *l = stakk; l && left; l = l->next) {
	for (GList *l = stakk; l != NULL; l = l->next) {
		tab = l->data;
		if (tab->xccv && tab->xccv->atv) {
			answer->xccv = tab->xccv;
			left--;
		}
		else if (current == tab->gen) {
			answer->gen = tab->gen;
			left--;
		}
		if (! left)
			break;
	}
	return answer;
}


static void
cb_copy (GSimpleAction *simple, GVariant *parameter, gpointer stck) {
	struct tabbit *tab = get_actives (GTK_STACK (stck));

	if (tab->gen)
		return;
	if (tab->xccv)
		xc_chat_view_copy_selection (tab->xccv);
	g_free (tab);
}


static void
example_destroy (GtkWidget *win, gpointer app) {
	struct tabbit *tab;

	GList *l;
	for (l = stakk; l != NULL; l = l->next) {
		tab = l->data;
		if (tab->xccv)
			g_object_unref (tab->xccv);
		if (tab->gen)
			gtk_widget_destroy (tab->gen);
	}

	g_application_quit (G_APPLICATION (app));
}


static void
cb_quit (GSimpleAction *simple, GVariant *parameter, gpointer win) {
	GtkApplication *app = gtk_window_get_application (win);
	example_destroy (win, app);
}

/*
xccv null
	gen?  switch
	xccv? detach/attach
xccv gen
	gen?  switch
	xccv? old_gen?  switch ; new xccv? detach attach
*/
void switch_tabs (GtkStack *stack, struct tabbit *old, struct tabbit *new) {
	if (new->gen) {
		gtk_stack_set_visible_child (stack, new->gen);
		return;
	}
	if (old->xccv != new->xccv) {
		xc_chat_view_detach (old->xccv);
		xc_chat_view_attach (new->xccv, &Atv);
	}
	if (old->gen)
		gtk_stack_set_visible_child_name (stack, "xccv");
}


static void
chpg (GtkStack *stack, gboolean up) {
	struct tabbit *tabold = NULL, *tab = NULL;
	gboolean got = FALSE,
		sb_visible = gtk_search_bar_get_search_mode (searchbits[SI_BAR]);
	GtkWidget *current = gtk_stack_get_visible_child (stack);

	tabold = get_actives (stack);

	for (GList *l = stakk; l != NULL; ) {
		tab = l->data;
		if (got)
			break;
		if ((tab->xccv && tab->xccv->atv && ! tabold->gen) || current == tab->gen) {
			got = TRUE;
			if (up) {
				l = l->prev;
				continue;
			}
		}
		l = l->next;
	}

	switch_tabs (stack, tabold, tab);

	if (sb_visible && (tabold->xccv != tab->xccv)) {
		gtk_widget_show (XC_CHAT_VIEW (tab->xccv)->search_widget);
		gtk_widget_hide (tabold->xccv->search_widget);
	}
	g_free (tabold);
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
init_atv (GtkWidget *stack) {
	GtkWidget *tview = gtk_tree_view_new ();
	GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (sw), tview);
	gtk_stack_add_named (GTK_STACK (stack), sw, "xccv");

	Atv.tview = GTK_TREE_VIEW (tview);
	Atv.sw = GTK_SCROLLED_WINDOW (sw);
}


static void
create_tabs (XcChatView *xccv, GtkWidget *stack) {
	struct tabbit *newtab = g_new0 (struct tabbit, 1);
	newtab->xccv = xccv;
	stakk = g_list_prepend (stakk, newtab);

	gtk_box_pack_start (searchbits[SI_LABEL], xccv->search_widget, FALSE, FALSE, 0);
}


static void
create_gen_tab (GtkWidget *wid, GtkWidget *stack, gchar *name) {
	struct tabbit *newtab = g_new0 (struct tabbit, 1);
	newtab->gen = wid;
	stakk = g_list_prepend (stakk, newtab);
	gtk_stack_add_named (GTK_STACK (stack), wid, name);
}


static void
cb_find (GSimpleAction *simple, GVariant *parameter, gpointer stack) {
	gboolean visible = gtk_search_bar_get_search_mode (searchbits[SI_BAR]);
	struct tabbit *tab = get_actives (stack);

	if (tab->gen && ! visible)
		return;
	gtk_search_bar_set_search_mode (searchbits[SI_BAR], ! visible);

	if (! tab->gen && ! visible)
		gtk_widget_show (tab->xccv->search_widget);
	else
		gtk_widget_hide (tab->xccv->search_widget);
// gtk_entry_set_icon_from_icon_name (searchbits[SI_ENTRY], GTK_ENTRY_ICON_SECONDARY, "dialog-error");
	g_free (tab);
}

static void
run_search (GtkSearchEntry *entry, gpointer stck) {
	GError *err = NULL;
	GtkStack *stack = stck;
	struct tabbit *tab = get_actives (stack);

	xc_chat_view_run_search (tab->xccv, gtk_entry_get_text (GTK_ENTRY (entry)), search_flags, &err);

	if (err) {
		g_print ("Search error: %s\n", err->message);
		g_error_free (err);
	}
	g_free (tab);
}

static void
next_search (gpointer *stack, gboolean direction) {
	struct tabbit *tab = get_actives (GTK_STACK (stack));
	xc_chat_view_next_search (tab->xccv, direction);
	g_free (tab);
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

	search_flags =	searchflags[SI_REGEX] ? SF_REGEXP     : 0 |
			searchflags[SI_CASE]  ? SF_CASE_MATCH : 0 |
			searchflags[SI_ALL]   ? SF_HIGHLIGHT  : 0 ;

	if (c != SI_ALL) // no highlight for now
		run_search (searchbits[SI_ENTRY], stack);
}

static void
cb_lastlog (GtkButton *togged, gpointer stck) {
	GtkStack *stack = GTK_STACK (stck);
	struct tabbit *tab = get_actives (stack);

	if (Lastlog == NULL) {
		Lastlog = xc_chat_view_new ();
		create_tabs (Lastlog, stck);
		gtk_widget_show_all (stck);
	}
	xc_chat_view_lastlog (tab->xccv, Lastlog, gtk_entry_get_text (searchbits[SI_ENTRY]), search_flags);
	g_free (tab);
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

	g_signal_connect (searchbits[SI_ENTRY],	"search-changed", G_CALLBACK (run_search), stack);
	g_signal_connect (searchbits[SI_ENTRY],	"activate",	G_CALLBACK (run_search), stack);
	g_signal_connect (searchbits[SI_ALL],	"toggled",	G_CALLBACK (cb_toggled), stack);
	g_signal_connect (searchbits[SI_CASE],	"toggled",	G_CALLBACK (cb_toggled), stack);
	g_signal_connect (searchbits[SI_REGEX],	"toggled",	G_CALLBACK (cb_toggled), stack);
	g_signal_connect (searchbits[SI_PREV],	"clicked",	G_CALLBACK (cb_bprev), stack);
	g_signal_connect (searchbits[SI_NEXT],	"clicked",	G_CALLBACK (cb_bnext), stack);

	g_signal_connect (lastlog, "clicked", G_CALLBACK (cb_lastlog), stack);
}

static void
cb_time (GSimpleAction *simple, GVariant *parameter, gpointer stack) {
	struct tabbit *tab = get_actives (stack);
	if (tab->gen)
		return;

	GVariant *state = g_action_get_state (G_ACTION (simple));
	gboolean flag = g_variant_get_boolean (state);
	g_variant_unref (state);

	g_simple_action_set_state (simple, g_variant_new_boolean (!flag));
	xc_chat_view_set_time_stamp (tab->xccv, !flag);
	g_free (tab);
}

static void
cb_save (GSimpleAction *simple, GVariant *parameter, gpointer stack) {
	gchar *file = "ssssssssss";
	struct tabbit *tab = get_actives (stack);
	if (tab->gen)
		return;

	gint fh = g_open (file, O_TRUNC | O_WRONLY | O_CREAT, 0600);
	if (fh != -1) {
		xc_chat_view_save (tab->xccv, fh);
		close (fh);
	}
	g_free (tab);
}

static void
cb_wrap (GSimpleAction *simple, GVariant *parameter, gpointer stack) {
	struct tabbit *tab = get_actives (stack);
	if (tab->gen)
		return;

	GVariant *state = g_action_get_state (G_ACTION (simple));
	gboolean flag = g_variant_get_boolean (state);
	g_variant_unref (state);
	g_simple_action_set_state (simple, g_variant_new_boolean (!flag));

	xc_chat_view_set_wordwrap (tab->xccv, !flag);
	g_free (tab);
}

static void
cb_clos (GSimpleAction *simple, GVariant *parameter, gpointer stack) {
	struct tabbit *tab = NULL;
	GtkWidget *current = gtk_stack_get_visible_child (stack);

	if (tab->gen) {
		return;
	}

	GList *l;
	for (l = stakk; l != NULL; l = l->next) {
		tab = l->data;
		if (current == tab->gen) { // TODO: fix this to phup/dn to adj xccv/stack
			gtk_widget_destroy (tab->gen);
			g_free (l->data);
			stakk = g_list_delete_link (stakk, l);
		}
	}
			xc_chat_view_detach (tab->xccv);
			g_object_unref (tab->xccv);
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

	//settings = g_settings_new ("com.github.example");

	GMenu *mmenu = g_menu_new ();
	GMenu *smenu = g_menu_new ();
	create_menu_item (smenu, "_Copy",	"app.copy",	NULL,	"<Primary>c",	NULL);
	create_menu_item (smenu, "Sa_ve",	"app.save",	NULL,	"<Primary><Shift>v",	NULL);
	create_menu_item (smenu, "_Word Wrap",	"app.wrap",	NULL,	"<Primary><Shift>w",	NULL);
	create_menu_item (smenu, "Show _Times",	"app.time",	NULL,	"<Primary>t",	NULL);
	create_menu_item (smenu, "_Find",	"app.find",	NULL,	"<Primary>f",	NULL);
	create_menu_item (smenu, "Find _Next",	"app.next",	NULL,	"<Primary>g",	NULL);
	create_menu_item (smenu, "Find _Prev",	"app.prev",	NULL,	"<Primary><Shift>g",	NULL);
	create_menu_item (smenu, "Page _Up",	"app.pgup",	NULL,	"<Primary>Prior",	NULL);
	create_menu_item (smenu, "Page _Down",	"app.pgdn",	NULL,	"<Primary>Next",	NULL);
	create_menu_item (smenu, "C_lose",	"app.clos",	NULL,	"<Primary>w",	NULL);
	create_menu_item (smenu, "_Quit",	"app.quit",	"application-exit",
									"<Primary>q",	NULL);
	g_menu_append_submenu (mmenu, "E_xample", G_MENU_MODEL (smenu));
	gtk_application_set_menubar (app, G_MENU_MODEL (mmenu));
	g_object_unref (mmenu);
	g_object_unref (smenu);

	GtkWidget *stack = gtk_stack_new ();
	init_atv (stack);

	GtkWidget *sbar = gtk_search_bar_new ();
	create_searchbar (sbar, stack);
	gtk_box_pack_start (GTK_BOX (mbox), stack,  TRUE,  TRUE, 0);
	gtk_box_pack_start (GTK_BOX (mbox),  sbar, FALSE, FALSE, 0);

	const GActionEntry wacts[] = {
		{"quit", cb_quit, NULL, NULL, NULL, {0, 0, 0}},
	};
	const GActionEntry sacts[] = {
		{"copy", cb_copy, NULL,    NULL, NULL, {0, 0, 0}},
		{"save", cb_save, NULL,    NULL, NULL, {0, 0, 0}},
		{"wrap", cb_wrap, NULL, "false", NULL, {0, 0, 0}},
		{"time", cb_time, NULL,  "true", NULL, {0, 0, 0}},
		{"next", cb_next, NULL,    NULL, NULL, {0, 0, 0}},
		{"prev", cb_prev, NULL,    NULL, NULL, {0, 0, 0}},
		{"pgup", cb_pgup, NULL,    NULL, NULL, {0, 0, 0}},
		{"pgdn", cb_pgdn, NULL,    NULL, NULL, {0, 0, 0}},
		{"clos", cb_clos, NULL,    NULL, NULL, {0, 0, 0}},
		{"find", cb_find, NULL,    NULL, NULL, {0, 0, 0}},
	};

	g_action_map_add_action_entries (G_ACTION_MAP (app), wacts, G_N_ELEMENTS (wacts), win);
	g_action_map_add_action_entries (G_ACTION_MAP (app), sacts, G_N_ELEMENTS (sacts), stack);

	GtkCssProvider *cssProvider = gtk_css_provider_new ();
	gtk_css_provider_load_from_path (cssProvider, "theme.css", NULL);
	gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
		GTK_STYLE_PROVIDER (cssProvider), GTK_STYLE_PROVIDER_PRIORITY_USER);

	gtk_widget_show_all (win);

	XcChatView *xccv1 = xc_chat_view_new ();
	create_tabs (xccv1, stack);
	xc_chat_view_tview_init (xccv1, &Atv);
	xc_chat_view_attach (xccv1, &Atv);
	xc_chat_view_set_handle_width (xccv1, TRUE, 15);

	XcChatView *xccv2 = xc_chat_view_new ();
	create_tabs (xccv2, stack);

	GtkWidget *txt1 = gtk_text_view_new ();
	GtkWidget *ssw = gtk_scrolled_window_new (NULL, NULL);
	gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (txt1)), "Hello world!\n", -1);
	gtk_container_add (GTK_CONTAINER (ssw), txt1);
	create_gen_tab (ssw, stack, "foo1");

	XcChatView *xccv3 = xc_chat_view_new ();
	create_tabs (xccv3, stack);

	gtk_widget_show_all (stack);
/*
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

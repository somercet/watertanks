/* Copyright 2023-2024 somercet
 * Released under GPL2.
 * Author: google mail com at somercet */

#ifndef __GTK_H__
#include <gtk/gtk.h>
#endif
// #include <glib/gprintf.h>

#ifndef _xc_search_flags_h_
#include "xc_search_flags.h"
#endif
#ifndef _xc_chat_view_h_
#include "xcchatview.h"
#endif

/*
enum {
	SEARCH_RESULT_CREATED,
	LAST_SIGNAL
};

static guint
xc_chat_view_signals[LAST_SIGNAL] = { 0 };
*/

/* static func declarations */
static void	cell_func_dtime (	GtkTreeViewColumn	*tree_column,
					GtkCellRenderer		*cell,
					GtkTreeModel		*tree_model,
					GtkTreeIter		*iter,
					gpointer		data );
static void	xc_chat_view_init (	XcChatView		*xccv );
static void	xc_chat_view_class_init	(XcChatViewClass	*class );
static void	xc_chat_view_dispose (	GObject			*object );
static void	xc_chat_view_load_scrollback_finish (GObject *sourceobject, GAsyncResult *result, gpointer userdata);
static void	xc_chat_view_load_scrollback_run (GTask *task, gpointer sobj, gpointer ud_file, GCancellable *cancellable);
static void	xc_chat_view_update_line_count (XcChatView *xccv, gint lines);
static void	xc_chat_view_set_wordwrap_real (XcChatView *xccv);
static void	xc_chat_view_clear_search (XcChatView *xccv);
static void	xc_chat_view_update_search_widget (XcChatView *xccv);
static void	tview_reparented (GtkWidget *tview, GtkWidget *old_parent, gpointer us);
static gboolean	is_scrolled_down (XcChatView *xccv);
static void	cb_mapped (GtkWidget *tview, gpointer user_data);

G_DEFINE_TYPE(XcChatView, xc_chat_view, G_TYPE_OBJECT)


// maingui.c	1. add a tabbed channel 2. create topwindow
// xtext_buffer *gtk_xtext_buffer_new (GtkXText *xtext);
XcChatView *
xc_chat_view_new (void)
{
  return g_object_new (XC_TYPE_CHAT_VIEW, NULL);
}


static void
xc_chat_view_init (XcChatView *xccv)
{
  xccv->dtformat = g_strdup ("%H:%M:%S");

  /* initialisation goes here */
  xccv->parent_widget = NULL;
  xccv->parent_reparent_cb_id = 0;
  xccv->parent_vadj = NULL;
  xccv->word_wrap = FALSE;
  xccv->word_wrap_width = -2;
  xccv->cell_td = gtk_cell_renderer_text_new ();
  xccv->cell_hn = gtk_cell_renderer_text_new ();
  xccv->cell_ms = gtk_cell_renderer_text_new ();
  xccv->store = gtk_list_store_new (SFS_COLNUM,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_DATE_TIME);
  xccv->tview = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (xccv->store)));
  xccv->clippy = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  xccv->select = gtk_tree_view_get_selection (xccv->tview);

  g_object_ref_sink (xccv->cell_td);
  g_object_ref_sink (xccv->cell_hn);
  g_object_ref_sink (xccv->cell_ms);
  g_object_ref_sink (xccv->tview);
/*
  GdkRGBA foo = { 0.0, 0.0, 0.0, 0.0 }; // { R, G, B, A } 0.0 to 1.0 double
  g_object_set (xccv->cell_ms, "background-rgba", &foo, NULL);
*/
  g_object_set (xccv->cell_td, "font", "Monospace 10", NULL);
  g_object_set (xccv->cell_ms, "wrap-mode", PANGO_WRAP_WORD_CHAR, NULL); // PANGO_WRAP_WORD, PANGO_WRAP_CHAR
  gtk_tree_view_set_headers_visible (xccv->tview, FALSE);

  gtk_tree_view_insert_column_with_data_func  (xccv->tview, TVC_TIMED,       "Date", xccv->cell_td,
						cell_func_dtime, xccv, NULL);
  gtk_tree_view_insert_column_with_attributes (xccv->tview, TVC_HANDLE,    "Handle", xccv->cell_hn, "text", SFS_HANDLE, NULL);
  gtk_tree_view_insert_column_with_attributes (xccv->tview, TVC_MESSAGE, "Messages", xccv->cell_ms, "text", SFS_MESSAG, NULL);
  gtk_tree_selection_set_mode (xccv->select, GTK_SELECTION_MULTIPLE);
  gtk_tree_view_set_enable_search (xccv->tview, FALSE);

  xccv->lines_max = 1000;
  xccv->lines_current = 0;
  xccv->timestamps = TRUE;

  g_mutex_init (&xccv->mutex);

  xccv->search_widget = gtk_label_new (NULL);
  gtk_widget_set_name (xccv->search_widget, "search_results");
  gtk_label_set_width_chars (GTK_LABEL (xccv->search_widget), 9);
  g_object_ref_sink (G_OBJECT (xccv->search_widget));
  xccv->search_label = g_string_new ("---");
  xc_chat_view_update_search_widget (xccv);

  xccv->marker_pos = NULL;
  xccv->marker_state = MARKER_WAS_NEVER_SET;

  xccv->parent_reparent_cb_id = g_signal_connect (xccv->tview, "parent-set", G_CALLBACK (tview_reparented), xccv);
  g_signal_connect (xccv->tview, "map", G_CALLBACK (cb_mapped), xccv);
}


static void
xc_chat_view_class_init (XcChatViewClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  /* virtual function overrides go here */
  gobject_class->dispose = xc_chat_view_dispose;

  /* property and signal definitions go here */

/*
  xc_chat_view_signals[SEARCH_RESULT_CREATED] = g_signal_new("search-result-created",
	XC_TYPE_CHAT_VIEW,	// G_TYPE_FROM_INSTANCE(xccv),
	G_SIGNAL_RUN_LAST,
	0,		// class_offset *fp
	NULL,		// accumulator
	NULL,		// accu data
	NULL,		// GSignalCMarshaller
	G_TYPE_NONE,	// return type
	0);		// param #
		// params G_TYPE_UINT);
*/
}


static void
xc_chat_view_dispose (GObject *object)
{
  XcChatView *xccv = XC_CHAT_VIEW (object);

  if (GTK_IS_TREE_VIEW (xccv->tview))
	gtk_widget_destroy (GTK_WIDGET (xccv->tview));
  g_clear_object (&xccv->tview);
  g_clear_object (&xccv->store);
  g_clear_object (&xccv->cell_td);
  g_clear_object (&xccv->cell_hn);
  g_clear_object (&xccv->cell_ms);

  if (xccv->scrollback_filename)
    g_free (xccv->scrollback_filename);
  if (xccv->dtformat)
    g_free (xccv->dtformat);

  g_mutex_clear (&xccv->mutex);

  if (xccv->search_paths)
  {
    g_list_free_full (xccv->search_paths, (GDestroyNotify) gtk_tree_path_free);
    xccv->search_paths = NULL;
  }
  g_string_free (xccv->search_label, TRUE);
  gtk_widget_destroy (xccv->search_widget);
  xccv->search_widget = NULL;

  G_OBJECT_CLASS (xc_chat_view_parent_class)->dispose (object);
}

static void
cell_func_dtime (	GtkTreeViewColumn	*tree_column,
			GtkCellRenderer	*cell,
			GtkTreeModel	*tree_model,
			GtkTreeIter	*iter,
			gpointer	xccvah)
{
  XcChatView *xccv = xccvah;
  GDateTime *dtime;

  gtk_tree_model_get (tree_model, iter, SFS_GDTIME, &dtime, -1);
  if (! dtime)
  {
    g_object_set (cell, "text", "", NULL);
    return;
  }

  g_object_set (cell, "text", g_date_time_format (dtime, xccv->dtformat), NULL);

  g_date_time_unref (dtime);
}


// TODO: push the difference between append and append_indent
// out to the calling code, which can set NULL as well as xccv.
void
xc_chat_view_append (XcChatView *xccv, guchar *mssg, int len, time_t stamp)
{
  xc_chat_view_append_indent (xccv, NULL, 0, mssg, len, stamp);
}


// TODO: pass in GDT, so can be NULL
void
xc_chat_view_append_indent (XcChatView *xccv,
	guchar	*hndl,	gint hndl_len,
	guchar	*mssg,	gint mssg_len,
	time_t	stamp)
{
  GDateTime	*gdt = NULL;
  GTimeZone	*tz = NULL;
  GtkTreeIter	iter;
  gboolean	down;
  gchar *h, *m;

  if (stamp == 0) {
    tz = g_time_zone_new_local ();
    gdt = g_date_time_new_now (tz);
  } else
    gdt = g_date_time_new_from_unix_local (stamp);
/*
  if (stamp != 0)
    gdt = g_date_time_new_from_unix_local (stamp);
*/

  if (mssg_len > 0 && mssg[mssg_len - 1] == '\n')
    mssg_len--;

  if (hndl_len == -1)
    h = g_strdup ((gchar *) hndl);
  else
    h = g_strndup ((gchar *) hndl, hndl_len);

  if (mssg_len == -1)
    m = g_strdup ((gchar *) mssg);
  else
    m = g_strndup ((gchar *) mssg, mssg_len);

  down = is_scrolled_down (xccv);

  g_mutex_init (&xccv->mutex);
  gtk_list_store_append (xccv->store, &iter);
  gtk_list_store_set (xccv->store, &iter,
    SFS_HANDLE, h,
    SFS_MESSAG, m,
    SFS_GDTIME, gdt,
    -1);

  xc_chat_view_update_line_count (xccv, 1);
  g_mutex_clear (&xccv->mutex);

  if (down)
    xc_chat_view_push_down_scrollbar (xccv);

  if (tz)
    g_time_zone_unref (tz);
  g_date_time_unref (gdt);
  g_free (h);
  g_free (m);
}

void
xc_chat_view_prepend0 (	XcChatView	*xccv,
			GDateTime	*dtime,
			gchar		*handle,
			gchar		*message ) {
  gtk_list_store_insert_with_values (xccv->store, NULL, 0,
    SFS_HANDLE, handle,
    SFS_MESSAG, message,
    SFS_GDTIME, dtime,
    -1);

  xc_chat_view_update_line_count (xccv, 1);
}


void
xc_chat_view_append0 (	XcChatView	*xccv,
			GDateTime	*dtime,
			gchar		*handle,
			gchar		*message ) {
  GtkTreeIter	iter;
  gboolean	down;

  down = is_scrolled_down (xccv);

  gtk_list_store_append (xccv->store, &iter);
  gtk_list_store_set (xccv->store, &iter,
    SFS_HANDLE, handle,
    SFS_MESSAG, message,
    SFS_GDTIME, dtime,
    -1);

  xc_chat_view_update_line_count (xccv, 1);

  if (down)
    xc_chat_view_push_down_scrollbar (xccv);
}


static void
xc_chat_view_update_line_count (XcChatView *xccv, gint l) {
	gint c;

	xccv->lines_current += l;
	c = xccv->lines_current - xccv->lines_max;
	if (c > 0)
		xc_chat_view_clear (xccv, c);
}


void
xc_chat_view_clear (XcChatView	*xccv, gint lines) {
	GtkTreeIter iter;
	gint c;

	if (lines < 0) // from bottom, TODO: "/CLEAR -nnn" command
		;
	else if (lines > 0) { // from top
		gtk_tree_model_get_iter_first (GTK_TREE_MODEL (xccv->store), &iter);
		for (c = lines; c > 0; c--) {
			if (! gtk_list_store_remove (xccv->store, &iter))
				g_printerr ("not deleted on %d of %d lines.\n", c, lines);
			else
				xccv->lines_current--;
		}
	} else { // all
		gtk_list_store_clear (xccv->store);
		xccv->lines_current = 0;
	}
}


// TODO: this should be a void
int
xc_chat_view_set_font (XcChatView *xccv, char *name)
{
  PangoFontDescription	*fontdesc;
  fontdesc = pango_font_description_from_string (name);
  if (!fontdesc)
	return 0;

  g_object_set (xccv->cell_td, "font-desc", pango_font_description_from_string ("Monospace 10"), NULL);
  g_object_set (xccv->cell_hn, "font-desc", fontdesc, NULL);
  g_object_set (xccv->cell_ms, "font-desc", fontdesc, NULL);

  pango_font_description_free (fontdesc);
  return 1;
}


static void
cb_mapped (GtkWidget *tview, gpointer user_data) {
	XcChatView *xccv = user_data;

	xc_chat_view_set_wordwrap (xccv, xccv->word_wrap);
}


static void
xc_chat_view_set_wordwrap_real (XcChatView *xccv) {
	gboolean down = FALSE;

	if (is_scrolled_down (xccv))
		down = TRUE;
	g_object_set (xccv->cell_ms, "wrap-width", xccv->word_wrap_width, NULL);
	gtk_tree_view_column_queue_resize (gtk_tree_view_get_column (xccv->tview, TVC_MESSAGE));
	if (down)
		xc_chat_view_push_down_scrollbar (xccv);
}

/*
states on map-event
  WW	OFF	  ON	 OFF	ON
 WWW	 -1	~500	~500	-1
WW is current or intended
WWW is actual
*/

void
xc_chat_view_set_wordwrap (XcChatView *xccv, gboolean word_wrap) {
	GtkTreeViewColumn *col;
	gint wcl;

	xccv->word_wrap = word_wrap;

	if (xccv->word_wrap) {
#ifdef GTK3
		wcl = gtk_widget_get_allocated_width (GTK_WIDGET (xccv->tview));
#else
		GtkAllocation rect;
		gtk_widget_get_allocation (GTK_WIDGET (xccv->tview), &rect);
		wcl = rect.width;
#endif
		if (wcl == 1) // not mapped yet
			return;
		if (xccv->timestamps) {
			col = gtk_tree_view_get_column (xccv->tview, TVC_TIMED);
			wcl -= gtk_tree_view_column_get_width (col);
		}
		col = gtk_tree_view_get_column (xccv->tview, TVC_HANDLE);
		wcl -= gtk_tree_view_column_get_width (col);
		wcl -= 8; // padding tween columns
		if (xccv->word_wrap_width == wcl)
			return;
		else
			xccv->word_wrap_width = wcl;
	} else {
		if (xccv->word_wrap_width == -1)
			return;
		else
			xccv->word_wrap_width = -1;
	}

	xc_chat_view_set_wordwrap_real (xccv);
}


void
xc_chat_view_set_time_stamp (XcChatView *xccv, gboolean show_dtime)
{
  GtkTreeViewColumn *dtime = gtk_tree_view_get_column (xccv->tview, TVC_TIMED);
  gtk_tree_view_column_set_visible (dtime, show_dtime);
  xccv->timestamps = show_dtime;
  if (xccv->word_wrap)
    xc_chat_view_set_wordwrap (xccv, xccv->word_wrap);
}


void
xc_chat_view_set_background (XcChatView *xccv, gchar *file)
{
#ifdef GTK3
  GtkCssProvider	*provider;
  GdkDisplay	*display;
  GdkScreen	*screen;
  gchar		*full, *half,
		*f = "treeview.view { background-image: %s; }\n";


  provider = gtk_css_provider_new();
  display = gdk_display_get_default();
  screen = gdk_display_get_default_screen(display);
  gtk_style_context_add_provider_for_screen (screen,
						GTK_STYLE_PROVIDER (provider),
						GTK_STYLE_PROVIDER_PRIORITY_USER);
  if (strlen(file))
    half = g_strdup_printf ("url(\"%s\")", file);
  else
    half = g_strdup ("none");

  full = g_strdup_printf (f, half);
  gtk_css_provider_load_from_data (provider, full, -1, NULL);
  g_object_unref (provider);
  g_free (half);
  g_free (full);
#else
	g_print ("File: '%s'.\n", file);
#endif
}


// TODO: All these can wait
void
/*
#ifdef GTK3
xc_chat_view_set_palette (XcChatView *xccv, GdkRGBA palette[])
#else
*/
xc_chat_view_set_palette (XcChatView *xccv, GdkColor palette[])
//#endif
{
// https://developer-old.gnome.org/gdk3/stable/gdk3-Colors.html#GdkColor
// https://developer-old.gnome.org/gdk3/stable/gdk3-RGBA-Colors.html#GdkRGBA
// GdkColor deprecated since 3.14
  return;
}
void
xc_chat_view_reset_marker_pos (XcChatView *xccv)
{
  return;
}
int
xc_chat_view_moveto_marker_pos (XcChatView *xccv)
{
  return MARKER_IS_SET;
}
void
xc_chat_view_check_marker_visibility (XcChatView *xccv)
{
// if (gtk_xtext_check_ent_visibility (xtext, xtext->buffer->marker_pos, 1))
//	xtext->buffer->marker_seen = TRUE;
  return;
}
void
xc_chat_view_set_show_marker (XcChatView *xccv, gboolean show_marker)
{
  return;
}
void
xc_chat_view_set_show_separator (XcChatView *xccv, gboolean show_separator)
{
  return;
}
void
xc_chat_view_set_thin_separator (XcChatView *xccv, gboolean thin_separator)
{
  return;
}
void
xc_chat_view_set_indent (XcChatView *xccv, gboolean indent)
{
  return;
}
void
xc_chat_view_set_max_indent (XcChatView *xccv, int max_auto_indent)
{
  return;
}

void
xc_chat_view_set_marker_last (gpointer sessresbuff) {
	XcChatView *xccv = G_TYPE_CHECK_INSTANCE_CAST (sessresbuff, XC_TYPE_CHAT_VIEW, XcChatView);
	GtkTreeModel *model = GTK_TREE_MODEL (xccv->store);
	GtkTreePath *path = NULL;
	GtkTreeIter iter;

	if (xccv->marker_pos)
		gtk_tree_row_reference_free (xccv->marker_pos);

        if (gtk_tree_model_get_iter_first (model, &iter)) { // yes, should be last but I'm just adding the variables we'll need
		path = gtk_tree_model_get_path (model, &iter);
		xccv->marker_pos = gtk_tree_row_reference_new (model, path);
		xccv->marker_state = MARKER_IS_SET;
	}
	if (path)
		gtk_tree_path_free (path);
}


void
xc_chat_view_save (XcChatView *xccv, int fh) {
	GtkTreeModel *model = GTK_TREE_MODEL (xccv->store);
	GString	*hold = g_string_sized_new (1024);
	GtkTreeIter iter;
	gboolean valid;
	gchar *h, *m;

	if (! gtk_tree_model_get_iter_first (model, &iter))
		return;

	do {
		gtk_tree_model_get (model, &iter, SFS_HANDLE, &h,
						  SFS_MESSAG, &m, -1);

		if (h[0] == '\0' || h[0] == '*' || h[0] == '-')
			g_string_printf (hold, "%s\t%s\n", h, m);
		else
			g_string_printf (hold, "<%s>\t%s\n", h, m);

		write (fh, hold->str, hold->len);
		g_free (h);
		g_free (m);

		valid = gtk_tree_model_iter_next (model, &iter);
	} while (valid);

	g_string_free (hold, TRUE);
}


void
xc_chat_view_set_max_lines (XcChatView *xccv, int max_lines)
{
  xccv->lines_max = (gint) max_lines;
}

void
xc_chat_view_copy_selection (XcChatView *xccv)
{
  GtkTreeModel	*model;
  GtkTreeIter	iter;
  GList		*rows, *r;
  GString	*hold = g_string_new (NULL);
  GDateTime	*gd;
  gchar		*h, *m, *dt_str, *nl,
		*newl = "\n",  *blank = "";

  rows = gtk_tree_selection_get_selected_rows (xccv->select, &model);

  nl = blank;
  for (r = rows; r != NULL; r = r->next)
  {
    if (gtk_tree_model_get_iter (model, &iter, r->data))
    {
      gtk_tree_model_get (model, &iter,
				SFS_GDTIME, &gd,
				SFS_HANDLE, &h,
				SFS_MESSAG, &m, -1);
      g_string_append (hold, nl);
      if (xccv->timestamps)
      {
        if (gd)
        {
          dt_str = g_date_time_format (gd, xccv->dtformat);
          g_string_append_printf (hold, "%s\t", dt_str);
          g_free (dt_str);
        }
        else
          g_string_append (hold, "\t");
      }
      g_string_append_printf (hold, "<%s>\t%s", h, m);
      g_date_time_unref (gd);
      g_free (h);
      g_free (m);
      if (nl != newl)
          nl = newl;
    }
  }
  g_list_free_full (rows, (GDestroyNotify) gtk_tree_path_free);
  gtk_clipboard_set_text (xccv->clippy, hold->str, hold->len);
  g_string_free (hold, TRUE);
}

static void
xc_chat_view_load_scrollback_finish (GObject *sobj, GAsyncResult *res, gpointer loop) {
	XcChatView *xccv = XC_CHAT_VIEW (sobj);
	GTask *task = G_TASK (res);
	GError *error;

	if (! g_task_propagate_boolean (task, &error)) {
		GDateTime *dt = g_date_time_new_now_local ();
		GString *msg = g_string_new ("Reading scrollback file failed: ");
		g_string_append (msg, error->message);
		xc_chat_view_append0 (xccv, dt, "", msg->str);

		g_error_free (error);
		g_string_free (msg, TRUE);
	}

	xc_chat_view_push_down_scrollbar (xccv);
	g_object_unref (task);
}


static void
xc_chat_view_load_scrollback_run (GTask *task, gpointer sobj, gpointer ud_file, GCancellable *cancellable) {
	XcChatView *xccv = XC_CHAT_VIEW (sobj);
	GFile *file = G_FILE (ud_file);
	GError *error;
	char *contents;
	gsize length;

	if (g_file_load_contents (file, NULL, &contents, &length, NULL, &error)) // cancellable, etag
	{
		gchar **lines = g_strsplit (contents, "\n", 0);
		GTimeZone *tz = g_time_zone_new_local ();
		guint i; // gtk2, was in for()

		for (i = g_strv_length (lines); i > 0; i--) {
			if (xccv->lines_current >= xccv->lines_max)
				break;

			GDateTime *t = NULL;
			gchar **f = g_strsplit (lines[i - 1], "\t", 3);

			if (f == NULL || g_strv_length (f) < 3) {
				xc_chat_view_prepend0 (xccv, NULL, "", "");
				g_strfreev (f);
				continue;
			} else {
				GDateTime *dt = g_date_time_new_from_iso8601 (f[0], NULL);

				if (dt) {
					t = g_date_time_to_timezone (dt, tz);
					g_date_time_unref (dt);
				}
			}

			gchar *r = f[1]; // lose <uname_brackets>
			if (f[1][0] == '<') {
				r++;
				size_t z = strlen(f[1]);
				if (f[1][z - 1] == '>') {
					f[1][z - 1] = '\0';
				}
			}

			xc_chat_view_prepend0 (xccv, t, r, f[2]);

			if (f)
				g_strfreev (f);
			if (t)
				g_date_time_unref (t);
		}

		g_strfreev (lines);
		g_free (contents);
		g_time_zone_unref (tz);
		g_task_return_boolean (task, TRUE);
	} else {
		g_task_return_error (task, error);
	}

	g_object_unref (file);
}


void
xc_chat_view_set_scrollback_file (XcChatView *xccv, gchar *filename)
{
  GError *error = NULL;
  gboolean begin = FALSE;

  GFile *file = g_file_new_for_path (filename);
  GFileOutputStream *stream = g_file_create (file, G_FILE_CREATE_NONE, NULL, &error);
  if (! error)
  {
    g_object_unref (stream);
    begin = TRUE;
  } else if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
    g_error_free (error);
    begin = TRUE;
  }

  if (begin)
  {
    xccv->scrollback_filename = g_strdup (filename);
    GTask *task = g_task_new (xccv, NULL, xc_chat_view_load_scrollback_finish, file); // cancellable
    g_task_set_task_data (task, file, NULL);
    g_task_run_in_thread (task, xc_chat_view_load_scrollback_run);
  } else {
    GDateTime *dt = g_date_time_new_now_local ();
    GString *msg = g_string_new ("Set scrollback file failed: ");
    g_string_append_len (msg, error->message, -1);
    xc_chat_view_append0 (xccv, dt, "", msg->str);
    g_date_time_unref (dt);
    g_error_free (error);
    g_string_free (msg, TRUE);
  }
}


static void
xc_chat_view_clear_search (XcChatView *xccv) {
	g_string_assign (xccv->search_label, "---");
	xc_chat_view_update_search_widget (xccv);
	if (xccv->search_text) {
		g_free (xccv->search_text);
		xccv->search_text = NULL;
	}
	if (xccv->search_paths) {
		g_list_free_full (xccv->search_paths, (GDestroyNotify) gtk_tree_path_free);
		xccv->search_paths = NULL;
	}
	if (xccv->search_regex) {
		g_regex_unref (xccv->search_regex);
		xccv->search_regex = NULL;
	}
	xccv->search_current = NULL;
	xccv->search_flags = 0;
	xccv->search_total = 0;
	xccv->search_now = 0;
}

static void
xc_chat_view_update_search_widget (XcChatView *xccv) {
	gtk_label_set_text (GTK_LABEL (xccv->search_widget), xccv->search_label->str);
}

/* Nothing else calls search_handle_event() so we don't need
to process the PREV/NEXT buttons through this func. */
void
xc_chat_view_run_search (XcChatView *xccv, const gchar *stext, gint flags, GError **serr) {
	//GtkTreeModel *model = gtk_tree_view_get_model (xccv->tview);
	GtkTreeModel *model = GTK_TREE_MODEL (xccv->store);
	GString *hold = g_string_sized_new (1024);
	GDateTime *gd;
	GtkTreeIter iter;
	gboolean valid;
	gchar *dt_str, *h, *m, *temp;

	if (stext[0] == '\0') {
		gtk_tree_selection_unselect_all (xccv->select);
		g_string_assign (xccv->search_label, "---");
		xc_chat_view_update_search_widget (xccv);
		return;
	}

	if (g_strcmp0 (xccv->search_text, stext) == 0 && flags == xccv->search_flags)
		return;

	if (xccv->search_paths || xccv->search_text)
		xc_chat_view_clear_search (xccv);

	/* flag massage start */
	if (flags & SF_REGEXP) {
		xccv->search_regex = g_regex_new (stext, (flags & SF_CASE_MATCH) ? 0 : G_REGEX_CASELESS,
						  G_REGEX_MATCH_NOTEMPTY, serr);
		if (*serr)
			return;
		xccv->search_text = g_strdup (stext);
	} else if (flags & SF_CASE_MATCH)
		xccv->search_text = g_strdup (stext);
	else
		xccv->search_text = g_utf8_casefold (stext, -1);

	if (flags & SF_HIGHLIGHT) // ignore for now
		;
	/* flag massage end */

	if (! gtk_tree_model_get_iter_first (model, &iter)) {
		g_string_assign (xccv->search_label, "---");
		xc_chat_view_update_search_widget (xccv);
		return;
	}

	xccv->search_flags = flags;

	do {
		GMatchInfo *gmi = NULL;
		dt_str = "";


		gtk_tree_model_get (model, &iter,
				SFS_GDTIME, &gd,
				SFS_HANDLE, &h,
				SFS_MESSAG, &m, -1);
		if (xccv->timestamps && gd)
			dt_str = g_date_time_format (gd, xccv->dtformat);

		g_string_append_printf (hold, "%s %s %s", dt_str, h, m);

		if (!( flags & SF_CASE_MATCH) && !( flags & SF_REGEXP)) {
			temp = g_utf8_casefold (hold->str, -1);
			g_string_assign (hold, temp);
			g_free (temp);
		}

		if ((flags & SF_REGEXP && g_regex_match (xccv->search_regex, hold->str, 0, &gmi))
			|| (strstr (hold->str, xccv->search_text))) {
			GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
			xccv->search_paths = g_list_prepend (xccv->search_paths, path);
			xccv->search_total++;
		}

		if (gd) {
			g_date_time_unref (gd);
			if (xccv->timestamps)
				g_free (dt_str);
		}
		g_free (h);
		g_free (m);
		g_string_truncate(hold, 0);
		if (gmi)
			g_match_info_free (gmi);

		valid = gtk_tree_model_iter_next(model, &iter);
	} while (valid);

	if (xccv->search_paths)
		xc_chat_view_next_search (xccv, FALSE);
	else
		gtk_tree_selection_unselect_all (xccv->select);

	g_string_free (hold, TRUE);
	return;
}

/* false, down the window, or next; true, up the window, or prev.
The GList starts at the end. */
void
xc_chat_view_next_search (XcChatView *xccv, gboolean direction) {
	if (!xccv->search_paths)
		return;
	if (!xccv->search_current) { // new search
		xccv->search_current = xccv->search_paths;
		xccv->search_now = 1;
	} else if (direction && xccv->search_current->prev) {
		xccv->search_current = xccv->search_current->prev;
		xccv->search_now--;
	} else if (!direction && xccv->search_current->next) {
		xccv->search_current = xccv->search_current->next;
		xccv->search_now++;
	}

	if (xccv->search_current->data) {
		gtk_tree_selection_unselect_all (xccv->select);
		gtk_tree_selection_select_path (xccv->select, xccv->search_current->data);
		gtk_tree_view_scroll_to_cell (xccv->tview, xccv->search_current->data, NULL, FALSE, 0, 0);

		g_string_printf (xccv->search_label, "%d of %d", xccv->search_now, xccv->search_total);
		xc_chat_view_update_search_widget (xccv);
	}
}

// common/outbound.c:lastlog (session *sess, char *search, gtk_xtext_search_flags flags)
// fe-gtk.c:fe_lastlog (session *sess, session *lastlog_sess, char *sstr, gtk_xtext_search_flags flags)

/* fe-gtk.c doesn't even check the return value. */
void
xc_chat_view_lastlog (XcChatView *xccv, XcChatView *target, const gchar *text, gint flags) {
	GtkTreeModel *model = GTK_TREE_MODEL (xccv->store);
	GString *hold = g_string_sized_new (1024);
	GDateTime *gd;
	GtkTreeIter iter;
	gboolean valid;
	gchar *dt_str, *h, *m;

	xc_chat_view_clear (target, 0);

	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			dt_str = "";
			gtk_tree_model_get (model, &iter,
					SFS_GDTIME, &gd,
					SFS_HANDLE, &h,
					SFS_MESSAG, &m, -1);
			if (xccv->timestamps && gd)
				dt_str = g_date_time_format (gd, xccv->dtformat);

			g_string_append_printf (hold, "%s %s %s", dt_str, h, m);

			if (strstr (hold->str, text))
				xc_chat_view_append0 (target, gd, h, m);

			if (gd) {
				g_date_time_unref (gd);
				if (xccv->timestamps)
					g_free (dt_str);
			}
			g_free (h);
			g_free (m);
			g_string_truncate(hold, 0);
			valid = gtk_tree_model_iter_next(model, &iter);
		} while (valid);
	}
	g_string_free (hold, TRUE);
	return;
}

static void
tview_reparented (GtkWidget *tview, GtkWidget *old_parent, gpointer us) {
	XcChatView *xccv = XC_CHAT_VIEW (us);

	xccv->parent_widget = gtk_widget_get_parent (GTK_WIDGET (xccv->tview));

	if (!xccv->parent_widget) {
		xccv->parent_vadj = NULL;
		return;
	}
	if (xccv->parent_widget && GTK_IS_SCROLLED_WINDOW (xccv->parent_widget))
		xccv->parent_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (xccv->parent_widget));
}

static gboolean
is_scrolled_down (XcChatView *xccv) {
	gdouble page_size = gtk_adjustment_get_page_size (xccv->parent_vadj);
	if (page_size == 0.0)
		return TRUE; // not scrolling yet ... I think.
	gdouble value = gtk_adjustment_get_value (xccv->parent_vadj);
	gdouble upper = gtk_adjustment_get_upper (xccv->parent_vadj);

	if (value <= upper - page_size)
		return TRUE;
	else
		return FALSE;
}

void
xc_chat_view_push_down_scrollbar (XcChatView *xccv) {
	gdouble page_size = gtk_adjustment_get_page_size (xccv->parent_vadj);
	gdouble upper = gtk_adjustment_get_upper (xccv->parent_vadj);

	gtk_adjustment_set_value (xccv->parent_vadj, upper - page_size);
}


/*
xccv->parent_reparent_cb_id = g_signal_connect(xccv->parent_vadj, "changed", G_CALLBACK(on_adjustment_changed), xccv);

	if (old_parent && xccv->parent_reparent_cb_id != 0) {
		g_signal_handler_disconnect (xccv->tview, xccv->parent_reparent_cb_id);
		xccv->parent_reparent_cb_id = 0;
	}
		// xccv->parent_reparent_cb_id =
			// g_signal_connect (xccv->tview, "parent-set", G_CALLBACK (xc_chat_view_reparented), xccv);
	if (xccv->parent_widget) {
		xccv->parent_widget_cb_id = g_signal_connect (xccv->parent_widget,
				"size-allocate", G_CALLBACK(xc_chat_view_resizing), xccv);
	}


Abandoned attempt to get auto resizing of wordwrap.  May come in handy later.

static void	xc_chat_view_resizing (GtkWidget *scrolledw, GdkRectangle *alloc, gpointer us);

static void
xc_chat_view_resizing (GtkWidget *scrolledw, GdkRectangle *alloc, gpointer us) {
	XcChatView *xccv = XC_CHAT_VIEW (us);
	GtkTreeViewColumn *col;
	gint wcl;

	if (xccv->word_wrap && xccv->parent_widget) {
		wcl = gtk_widget_get_allocated_width (GTK_WIDGET (xccv->tview));

		col = gtk_tree_view_get_column (xccv->tview, TVC_TIMED);
		if (gtk_tree_view_column_get_visible (col))
			wcl -= gtk_tree_view_column_get_width (col);
		col = gtk_tree_view_get_column (xccv->tview, TVC_HANDLE);
		xccv->word_wrap_width = wcl - gtk_tree_view_column_get_width (col);

g_print ("test %d: %s %d\n", __LINE__, __FILE__, xccv->word_wrap_width);
		xc_chat_view_set_wordwrap_real (xccv);
	}
}
//					col = gtk_tree_view_get_column (xccv->tview, TVC_MESSAGE);
*/


/*
g_printerr ("test %d: %s\n", __LINE__, __FILE__);
g_printerr ("Widget: %s\n", G_OBJECT_TYPE_NAME (sess->gui->menu_item[id]));
g_printerr ("Class: %s\n", G_OBJECT_TYPE_NAME (sess->gui->menu_item[id]));
*/

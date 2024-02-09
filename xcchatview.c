/* Copyright 2023-2024 somercet
 * Released under GPL2.
 * Author: google mail com at somercet */

#ifndef __GTK_H__
#include <gtk/gtk.h>
#endif
// #include <glib/gprintf.h>
#ifndef _xc_chat_view_h_
#include "xcchatview.h"
#endif

extern GSettings *settings;

enum xc_chat_view_properties {
    PROP_DTFORMAT = 1,
    PROP_COUNT
};

static GParamSpec *xcproperties[PROP_COUNT] = { NULL, };

/* static func declarations */
static void	cell_func_dtime (	GtkTreeViewColumn	*tree_column,
					GtkCellRenderer		*cell,
					GtkTreeModel		*tree_model,
					GtkTreeIter		*iter,
					gpointer		data );
static void	xc_chat_view_init (	XcChatView		*xccv );
static void	xc_chat_view_class_init	(XcChatViewClass	*class );
static void	xc_chat_view_dispose (	GObject			*object );
static void	xc_chat_view_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void	xc_chat_view_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void	load_scrollback_finish (GObject *sourceobject, GAsyncResult *result, gpointer userdata);
static void	load_scrollback_run (GTask *task, gpointer sobj, gpointer ud_file, GCancellable *cancellable);

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
  /* initialisation goes here */
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
  g_object_ref_sink (xccv->store);
  g_object_ref_sink (xccv->tview);
/*
  GdkRGBA foo = { 0.0, 0.0, 0.0, 0.0 }; // { R, G, B, A } 0.0 to 1.0 double
  g_object_set (xccv->cell_ms, "background-rgba", &foo, NULL);
*/
  g_object_set (xccv->cell_td, "font", "Monospace 10", NULL);
  g_object_set (xccv->cell_ms, "wrap-mode", PANGO_WRAP_WORD_CHAR, NULL);
  gtk_tree_view_set_headers_visible (xccv->tview, FALSE);

  gtk_tree_view_insert_column_with_data_func  (xccv->tview, TVC_TIMED,       "Date", xccv->cell_td,
						cell_func_dtime, xccv, NULL);
  gtk_tree_view_insert_column_with_attributes (xccv->tview, TVC_HANDLE,    "Handle", xccv->cell_hn, "markup", SFS_HANDLE, NULL);
  gtk_tree_view_insert_column_with_attributes (xccv->tview, TVC_MESSAGE, "Messages", xccv->cell_ms, "markup", SFS_MESSAG, NULL);
  gtk_tree_selection_set_mode (xccv->select, GTK_SELECTION_MULTIPLE);
  gtk_tree_view_set_enable_search (xccv->tview, FALSE);

  //xccv->dtformat = g_strdup ("%F");
  g_settings_bind (settings, "stamp-text-format", xccv, "stamp-text-format", G_SETTINGS_BIND_GET);
}


static void
xc_chat_view_class_init (XcChatViewClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  /* virtual function overrides go here */
  gobject_class->dispose = xc_chat_view_dispose;
  gobject_class->get_property = xc_chat_view_get_property;
  gobject_class->set_property = xc_chat_view_set_property;

  /* property and signal definitions go here */
  xcproperties[PROP_DTFORMAT] = g_param_spec_string (
    "stamp-text-format",			// name
    "Timestamp format",				// nickname
    "See the strftime manpage for details.",	// description
    "%I%M%S",					// Default value
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, PROP_COUNT, xcproperties);
}


static void
xc_chat_view_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
  XcChatView *xccv = XC_CHAT_VIEW (object);

  switch (prop_id) {
    case PROP_DTFORMAT:
      if (xccv->dtformat)
        g_free (xccv->dtformat);
      xccv->dtformat = g_value_dup_string (value);
      gtk_tree_view_column_queue_resize (gtk_tree_view_get_column (xccv->tview, TVC_TIMED));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (xccv, prop_id, pspec);
      break;
  }
}


static void
xc_chat_view_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
  XcChatView *xccv = XC_CHAT_VIEW (object);

  switch (prop_id) {
    case PROP_DTFORMAT:
      g_value_set_string (value, xccv->dtformat);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
xc_chat_view_dispose (GObject *object)
{
  XcChatView *xccv = XC_CHAT_VIEW (object);

  g_clear_object (&xccv->tview);
  g_clear_object (&xccv->store);
  g_clear_object (&xccv->cell_td);
  g_clear_object (&xccv->cell_hn);
  g_clear_object (&xccv->cell_ms);

  if (xccv->scrollback_filename)
    g_free (xccv->scrollback_filename);
  if (xccv->dtformat)
    g_free (xccv->dtformat);

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
    g_object_set (cell, "markup", "", NULL);
    return;
  }

  g_object_set (cell, "markup", g_date_time_format (dtime, xccv->dtformat), NULL);

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
	guchar	*hndl,	int hndl_len,
	guchar	*mssg,	int mssg_len,
	time_t	stamp)
{
  GDateTime	*gdt;
  GtkTreeIter	iter;

  gdt = g_date_time_new_from_unix_local (stamp); // expects gint64

  if (mssg[mssg_len - 1] == '\n')
    mssg[mssg_len - 1] = '\0';

  gtk_list_store_append (xccv->store, &iter);
  gtk_list_store_set (xccv->store, &iter,
    SFS_HANDLE, hndl,
    SFS_MESSAG, mssg,
    SFS_GDTIME, gdt,
    -1);

  g_date_time_unref (gdt);
}


// TODO: lines <0 from bottom, 0 all, >0 from top
void
xc_chat_view_clear (XcChatView	*xccv, int lines)
{
  gtk_list_store_clear (xccv->store);
}


// TODO: this should be a void
int
xc_chat_view_set_font (XcChatView *xccv, char *name)
{
  PangoFontDescription	*fontdesc;
  fontdesc = pango_font_description_from_string (name);

  g_object_set (xccv->cell_td, "font-desc", pango_font_description_from_string ("Monospace 10"), NULL);
  g_object_set (xccv->cell_hn, "font-desc", fontdesc, NULL);
  g_object_set (xccv->cell_ms, "font-desc", fontdesc, NULL);

  pango_font_description_free (fontdesc);
  return 1;
}


void
xc_chat_view_set_wordwrap (XcChatView *xccv, gboolean word_wrap)
{
  int s = -1;

  if (word_wrap)
    s = 100;

  g_object_set (xccv->cell_ms, "wrap-width", s, NULL);
  gtk_tree_view_column_queue_resize (gtk_tree_view_get_column (xccv->tview, TVC_MESSAGE));
  // PANGO_WRAP_WORD, PANGO_WRAP_CHAR, PANGO_WRAP_WORD_CHAR
}


void
xc_chat_view_set_time_stamp (XcChatView *xccv, gboolean show_dtime)
{
  GtkTreeViewColumn *dtime = gtk_tree_view_get_column (xccv->tview, TVC_TIMED);
  gtk_tree_view_column_set_visible (dtime, show_dtime);
}


void
// xc_chat_view_set_background (XcChatView *xccv, GdkPixmap *pixmap )
xc_chat_view_set_background (XcChatView *xccv, gchar *file)
{
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
}


// TODO: All these can wait
void
//xc_chat_view_set_palette (XcChatView *xccv, GdkColor palette[])
xc_chat_view_set_palette (XcChatView *xccv, GdkRGBA palette[])
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
xc_chat_view_save (XcChatView *xccv, int fh)
{
  return;
}


void
xc_chat_view_set_max_lines (XcChatView *xccv, int max_lines)
{
  xccv->max_lines = (gint) max_lines;
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
  GtkTreeViewColumn *dtime = gtk_tree_view_get_column (xccv->tview, TVC_TIMED);

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
      if (gtk_tree_view_column_get_visible (dtime))
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
load_scrollback_finish (GObject *sobj, GAsyncResult *res, gpointer loop) {
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

	g_object_unref (task);
}


static void
load_scrollback_run (GTask *task, gpointer sobj, gpointer ud_file, GCancellable *cancellable) {
	XcChatView *xccv = XC_CHAT_VIEW (sobj);
	GFile *file = G_FILE (ud_file);
	GError *error;
	char *contents;
	gsize length;

	if (g_file_load_contents (file, NULL, &contents, &length, NULL, &error)) // cancellable, etag
	{
		gchar **lines = g_strsplit (contents, "\n", 0);
		GTimeZone *tz = g_time_zone_new_local ();

		for (guint i = g_strv_length (lines); i > 0; i--) {
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

			xc_chat_view_prepend0 (xccv, t, f[1], f[2]);

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
    GTask *task = g_task_new (xccv, NULL, load_scrollback_finish, file); // cancellable
    g_task_set_task_data (task, file, NULL);
    g_task_run_in_thread (task, load_scrollback_run);
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
}


void
xc_chat_view_append0 (	XcChatView	*xccv,
			GDateTime	*dtime,
			gchar		*handle,
			gchar		*message ) {
  GtkTreeIter	iter;

  gtk_list_store_append (xccv->store, &iter);
  gtk_list_store_set (xccv->store, &iter,
    SFS_HANDLE, handle,
    SFS_MESSAG, message,
    SFS_GDTIME, dtime,
    -1);
}

/*
struct search_results {
  gchar *search_text;
  GList *search_paths;
  GList *search_current;
};

static void
search_columns () {

}
*/

void
xc_chat_view_run_search (XcChatView *xccv, const gchar *stext, gboolean all, gboolean icase, gboolean regex) {
	GtkTreeModel *model = gtk_tree_view_get_model (xccv->tview);
	GString *holder = g_string_new("");
	GtkTreeIter iter;
	gboolean valid;

	xccv->search_current = NULL;
	if (xccv->search_paths != NULL) {
		g_list_free_full (xccv->search_paths, (GDestroyNotify) gtk_tree_path_free);
		xccv->search_paths = NULL;
	}

	if (stext[0] == '\0') {
		gtk_tree_selection_unselect_all (xccv->select);
		return;
	}

	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			gchar *value;
			for (int c = 0; c < TVC_TOTAL; c++) {
				gtk_tree_model_get (model, &iter, c, &value, -1);
				g_string_append_printf (holder, "%s%s", " ", value);
				g_free (value);
			}

			if (g_strrstr (holder->str, stext)) {
				GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
				xccv->search_paths = g_list_prepend (xccv->search_paths, path);
			}

			g_string_truncate(holder, 0);
			valid = gtk_tree_model_iter_next(model, &iter);
		} while (valid);
		if (xccv->search_paths)
			xc_chat_view_next_search (xccv, TRUE);
	}
	g_string_free (holder, TRUE);
	return; // String not found
}

/* false, down the window; true, up the window. The GList starts at the end. */
void
xc_chat_view_next_search (XcChatView *xccv, gboolean direction) {
	//gboolean move = FALSE;

	if (!xccv->search_paths)
		return;

	if (!xccv->search_current)
		xccv->search_current = xccv->search_paths; // new search
	else if (direction) {
		if (xccv->search_current->prev == NULL)
			return;
		else
			xccv->search_current = xccv->search_current->prev;
	} else if (!direction) {
		if (xccv->search_current->next == NULL)
			return;
		else
			xccv->search_current = xccv->search_current->next;
	}

	if (xccv->search_current->data) {
		gtk_tree_selection_unselect_all (xccv->select);
		gtk_tree_selection_select_path (xccv->select, xccv->search_current->data);
		gtk_tree_view_scroll_to_cell (xccv->tview, xccv->search_current->data, NULL, FALSE, 0, 0);
	}
}

/* fe-gtk.c doesn't even check the return value. */
void
xc_chat_view_lastlog (XcChatView *xccv, const gchar *text, XcChatView *target) {
	GtkTreeModel *model = gtk_tree_view_get_model (xccv->tview);
	GtkTreeIter iter;
	gboolean valid;
	GDateTime *gd;
	gchar *dt_str, *h, *m;
	GString *hold = g_string_new(NULL);
	GtkTreeViewColumn *dtime = gtk_tree_view_get_column (xccv->tview, TVC_TIMED);
	gboolean tdates = gtk_tree_view_column_get_visible (dtime);

	xc_chat_view_clear (target, 0);

	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			gtk_tree_model_get (model, &iter,
					SFS_GDTIME, &gd,
					SFS_HANDLE, &h,
					SFS_MESSAG, &m, -1);
			if (tdates) {
				if (gd) {
					dt_str = g_date_time_format (gd, xccv->dtformat);
					g_string_append_printf (hold, "%s ", dt_str);
					g_free (dt_str);
				}
			}
			g_string_append_printf (hold, "%s %s", h, m);

			if (g_strrstr (hold->str, text))
				xc_chat_view_prepend0 (target, gd, h, m);
			if (gd)
				g_date_time_unref (gd);
			g_free (h);
			g_free (m);
			g_string_truncate(hold, 0);
			valid = gtk_tree_model_iter_next(model, &iter);
		} while (valid);
	}
	g_string_free (hold, TRUE);
	return;
}


/*
g_print ("test %d: %s\n", __LINE__, __FILE__);
*/

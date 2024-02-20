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

#ifdef USE_GTK3
extern GSettings *settings;
#else
#endif

enum {
	SEARCH_RESULT_CREATED,
	LAST_SIGNAL
};

static guint
xc_chat_view_signals[LAST_SIGNAL] = { 0 };

enum xc_chat_view_properties {
    PROP_STAMP_TEXT = 1,
    PROP_STAMP_TEXT_FORMAT,
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
static void	xc_chat_view_load_scrollback_finish (GObject *sourceobject, GAsyncResult *result, gpointer userdata);
static void	xc_chat_view_load_scrollback_run (GTask *task, gpointer sobj, gpointer ud_file, GCancellable *cancellable);
static void	xc_chat_view_update_line_count (XcChatView *xccv, gint lines);
static void	xc_chat_view_set_wordwrap_real (XcChatView *xccv);
static void	xc_chat_view_clear_search (XcChatView *xccv);
static void	xc_chat_view_update_search_widget (XcChatView *xccv);


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

  g_mutex_init (&xccv->mutex);

  xccv->search_widget = gtk_label_new (NULL);
  gtk_widget_set_name (xccv->search_widget, "search_results");
  gtk_label_set_width_chars (GTK_LABEL (xccv->search_widget), 9);
  g_object_ref_sink (G_OBJECT (xccv->search_widget));
  xccv->search_label = g_string_new ("---");
  xc_chat_view_update_search_widget (xccv);

/*
  Search signals and GSettings not used for now.

  g_signal_emit (xccv, xc_chat_view_signals[SEARCH_RESULT_CREATED], 0);

*/
#ifdef USE_GTK3
  g_settings_bind (settings, "stamp-text",        xccv, "stamp-text",        G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (settings, "stamp-text-format", xccv, "stamp-text-format", G_SETTINGS_BIND_GET);
#else
#endif

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
  xcproperties[PROP_STAMP_TEXT] = g_param_spec_boolean (
    "stamp-text",				// name
    "Enable timestamps",			// nickname
    "Show timestamps in chat windows.",		// description
    FALSE,					// Default value
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  xcproperties[PROP_STAMP_TEXT_FORMAT] = g_param_spec_string (
    "stamp-text-format",			// name
    "Timestamp format",				// nickname
    "See the strftime manpage for details.",	// description
    "%I%M%S",					// Default value
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, PROP_COUNT, xcproperties);

/*
  Not doing search results this way, for now.
*/

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
}


static void
xc_chat_view_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
  XcChatView *xccv = XC_CHAT_VIEW (object);

  switch (prop_id) {
    case PROP_STAMP_TEXT:
      xccv->timestamps = g_value_get_boolean(value);
      xc_chat_view_set_time_stamp (xccv, xccv->timestamps);
      break;
    case PROP_STAMP_TEXT_FORMAT:
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
    case PROP_STAMP_TEXT:
      g_value_set_boolean (value, xccv->timestamps);
      break;
    case PROP_STAMP_TEXT_FORMAT:
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

  xc_chat_view_update_line_count (xccv, 1);

  g_date_time_unref (gdt);
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

  gtk_list_store_append (xccv->store, &iter);
  gtk_list_store_set (xccv->store, &iter,
    SFS_HANDLE, handle,
    SFS_MESSAG, message,
    SFS_GDTIME, dtime,
    -1);

  xc_chat_view_update_line_count (xccv, 1);
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

  g_object_set (xccv->cell_td, "font-desc", pango_font_description_from_string ("Monospace 10"), NULL);
  g_object_set (xccv->cell_hn, "font-desc", fontdesc, NULL);
  g_object_set (xccv->cell_ms, "font-desc", fontdesc, NULL);

  pango_font_description_free (fontdesc);
  return 1;
}


static void
xc_chat_view_set_wordwrap_real (XcChatView *xccv) {
	g_object_set (xccv->cell_ms, "wrap-width", xccv->word_wrap_width, NULL);
	gtk_tree_view_column_queue_resize (gtk_tree_view_get_column (xccv->tview, TVC_MESSAGE));
}


void
xc_chat_view_set_wordwrap (XcChatView *xccv, gboolean word_wrap)
{
	GtkTreeViewColumn *col;
	gint wcl;

	xccv->word_wrap = word_wrap;

	if (word_wrap) {
		//xccv->parent_widget = gtk_widget_get_parent (GTK_WIDGET (xccv->tview));
		//if (xccv->parent_widget)
#ifdef USE_GTK3
		wcl = gtk_widget_get_allocated_width (GTK_WIDGET (xccv->tview));
#else
		GtkAllocation rect;
		gtk_widget_get_allocation (GTK_WIDGET (xccv->tview), &rect);
		wcl = rect.width;
#endif
		col = gtk_tree_view_get_column (xccv->tview, TVC_TIMED);
		if (gtk_tree_view_column_get_visible (col))
			wcl -= gtk_tree_view_column_get_width (col);
		col = gtk_tree_view_get_column (xccv->tview, TVC_HANDLE);
		xccv->word_wrap_width = wcl - 10 - gtk_tree_view_column_get_width (col); // 10 == fudge value
	} else
		xccv->word_wrap_width = -1;

	xc_chat_view_set_wordwrap_real (xccv);
}


void
xc_chat_view_set_time_stamp (XcChatView *xccv, gboolean show_dtime)
{
  GtkTreeViewColumn *dtime = gtk_tree_view_get_column (xccv->tview, TVC_TIMED);
  gtk_tree_view_column_set_visible (dtime, show_dtime);
}


void
#ifdef USE_GTK3
xc_chat_view_set_background (XcChatView *xccv, gchar *file)
#else
xc_chat_view_set_background (XcChatView *xccv, GdkPixmap *pixmap )
#endif
{
#ifdef USE_GTK3
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
	return;
#endif
}


// TODO: All these can wait
void
#ifdef USE_GTK3
xc_chat_view_set_palette (XcChatView *xccv, GdkRGBA palette[])
#else
xc_chat_view_set_palette (XcChatView *xccv, GdkColor palette[])
#endif
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


/*
   gchar	*search_text;
   GList	*search_paths;
   GList	*search_current;
   guint	search_total;
   guint	search_now;
   GtkLabel	*search_widget; 
   GString	*search_label; 
   xc_search_flags       flags;
*/

static void
xc_chat_view_clear_search (XcChatView *xccv) {
	g_string_assign (xccv->search_label, "---");
	xc_chat_view_update_search_widget (xccv);
	g_free (xccv->search_text);
	if (xccv->search_paths)
		g_list_free_full (xccv->search_paths, (GDestroyNotify) gtk_tree_path_free);
	xccv->search_text = NULL;
	xccv->search_paths = NULL;
	xccv->search_current = NULL;
	xccv->search_flags = 0;
}

static void
xc_chat_view_update_search_widget (XcChatView *xccv) {
	gtk_label_set_text (GTK_LABEL (xccv->search_widget), xccv->search_label->str);
}

/* Nothing else calls search_handle_event() so we don't need
to process the PREV/NEXT buttons through this func. */
void
xc_chat_view_run_search (XcChatView *xccv, const gchar *stext, xc_search_flags flags) {
	//GtkTreeModel *model = gtk_tree_view_get_model (xccv->tview);
	GtkTreeModel *model = GTK_TREE_MODEL (xccv->store);
	GString *hold = g_string_sized_new (1024);
	GDateTime *gd;
	GtkTreeIter iter;
	gboolean valid;
	gchar *dt_str, *h, *m, *temp;

	if (xccv->search_paths != NULL)
		xc_chat_view_clear_search (xccv);

	if (stext[0] == '\0') {
		gtk_tree_selection_unselect_all (xccv->select);
		g_string_assign (xccv->search_label, "---");
		xc_chat_view_update_search_widget (xccv);
		return;
	}

	/* massage data start */
	if (flags & highlight) // ignore for now
		;

	if (flags & case_match)
		xccv->search_text = g_strdup (stext);
	else
		xccv->search_text = g_utf8_casefold (stext, -1);

	if (flags & regexp)
		;
	/* massage data end */

	xccv->search_flags = flags;

	if (gtk_tree_model_get_iter_first (model, &iter)) {
		xccv->search_total = 0;
		xccv->search_now = 0;
	} else {
		g_string_assign (xccv->search_label, "---");
		xc_chat_view_update_search_widget (xccv);
		return;
	}

	do {
		dt_str = "";
		gtk_tree_model_get (model, &iter,
				SFS_GDTIME, &gd,
				SFS_HANDLE, &h,
				SFS_MESSAG, &m, -1);
		if (xccv->timestamps && gd)
			dt_str = g_date_time_format (gd, xccv->dtformat);

		g_string_append_printf (hold, "%s %s %s", dt_str, h, m);

		if (! flags & case_match) {
			temp = g_utf8_casefold (hold->str, -1);
			g_string_assign (hold, temp);
			g_free (temp);
		}

		if (strstr (hold->str, xccv->search_text)) {
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

		valid = gtk_tree_model_iter_next(model, &iter);
	} while (valid);

	if (xccv->search_paths)
		xc_chat_view_next_search (xccv, FALSE);
	else
		gtk_tree_selection_unselect_all (xccv->select);

	g_string_free (hold, TRUE);
	return;
}

/* false, down the window; true, up the window. The GList starts at the end. */
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

/* fe-gtk.c doesn't even check the return value. */
void
xc_chat_view_lastlog (XcChatView *xccv, const gchar *text, XcChatView *target) {
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

/*
Abandoned attempt to get auto resizing of wordwrap.  May come in handy later.

static void	xc_chat_view_resizing (GtkWidget *scrolledw, GdkRectangle *alloc, gpointer us);
static void	xc_chat_view_reparented (GtkWidget *tview, GtkWidget *old_parent, gpointer us);


  g_signal_connect(xccv->tview, "parent-set", G_CALLBACK (xc_chat_view_reparented), xccv);


static void
xc_chat_view_reparented (GtkWidget *tview, GtkWidget *old_parent, gpointer us) {
	XcChatView *xccv = XC_CHAT_VIEW (us);

	if (old_parent)
		g_signal_handler_disconnect (xccv->parent_widget, xccv->parent_widget_cb_id);
	xccv->parent_widget = gtk_widget_get_parent (GTK_WIDGET (xccv->tview));
	// gtk_widget_get_window (GTK_WIDGET (xccv->tview));
	if (xccv->parent_widget) {
		xccv->parent_widget_cb_id = g_signal_connect (xccv->parent_widget,
				"size-allocate", G_CALLBACK(xc_chat_view_resizing), xccv);
}}


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
g_print ("test %d: %s\n", __LINE__, __FILE__);
*/

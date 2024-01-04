/* Copyright 2023 somercet
 * Released under GPL2.
 * Author: google mail com at somercet */

#ifndef __GTK_H__
#include <gtk/gtk.h>
#endif
// #include <glib/gprintf.h>
#ifndef _xc_chat_view_h_
#include "xcchatview.h"
#endif

/* static func declarations */
static void	cell_func_dtime (	GtkTreeViewColumn	*tree_column,
					GtkCellRenderer		*cell,
					GtkTreeModel		*tree_model,
					GtkTreeIter		*iter,
					gpointer		data );
static void	xc_chat_view_init (	XcChatView		*xccv );
static void	xc_chat_view_class_init	(XcChatViewClass	*class );
static void	xc_chat_view_dispose (	GObject			*object );

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
  xccv->dtformat = g_strdup ("%FT%T");
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
  g_object_set (xccv->cell_td, "font", "Monospace 10", NULL);
  g_object_set (xccv->cell_ms, "wrap-mode", PANGO_WRAP_WORD_CHAR, NULL);
  gtk_tree_view_set_headers_visible (xccv->tview, FALSE);

  gtk_tree_view_insert_column_with_data_func  (xccv->tview, TVC_TIMED,       "Date", xccv->cell_td,
						cell_func_dtime, xccv->dtformat, NULL);
  gtk_tree_view_insert_column_with_attributes (xccv->tview, TVC_HANDLE,    "Handle", xccv->cell_hn, "markup", SFS_HANDLE, NULL);
  gtk_tree_view_insert_column_with_attributes (xccv->tview, TVC_MESSAGE, "Messages", xccv->cell_ms, "markup", SFS_MESSAG, NULL);
  gtk_tree_selection_set_mode(xccv->select, GTK_SELECTION_MULTIPLE);
}


static void
xc_chat_view_class_init (XcChatViewClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  /* virtual function overrides go here */
  gobject_class->dispose = xc_chat_view_dispose;

  /* property and signal definitions go here */
}


static void
xc_chat_view_dispose (GObject *object)
{
  XcChatView *xccv = XC_CHAT_VIEW (object);

  if (xccv->tview)
    g_object_unref (xccv->tview);
  if (xccv->store)
    g_object_unref (xccv->store);
  if (xccv->cell_td)
    g_object_unref (xccv->cell_td);
  if (xccv->cell_hn)
    g_object_unref (xccv->cell_hn);
  if (xccv->cell_ms)
    g_object_unref (xccv->cell_ms);

  if (xccv->dtformat)
    g_free (xccv->dtformat);

  G_OBJECT_CLASS (xc_chat_view_parent_class)->dispose (object);
}



static void
cell_func_dtime (	GtkTreeViewColumn	*tree_column,
			GtkCellRenderer	*cell,
			GtkTreeModel	*tree_model,
			GtkTreeIter	*iter,
			gpointer	dtformat)
{
  GDateTime *dtime;
  gchar *result;

  gtk_tree_model_get (tree_model, iter, SFS_GDTIME, &dtime, -1);
  if (dtime == NULL)
    return;

  result = g_date_time_format (dtime, dtformat);
  g_object_set (cell, "markup", result, NULL);

  g_date_time_unref (dtime);
  g_free (result);
}


//	TODO: push the difference between append and append_indent
//	out to the calling code, which can set NULL as well as xccv.
void
xc_chat_view_append (XcChatView *xccv, guchar *mssg, int len, time_t stamp)
{
  xc_chat_view_append_indent (xccv, NULL, 0, mssg, len, stamp);
}


//	TODO: pass in GDT, so can be NULL
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


//	TODO: lines <0 from bottom, 0 all, >0 from top
void
xc_chat_view_clear (	XcChatView	*xccv,
			int		lines)
{
	gtk_list_store_clear (xccv->store);
}


//	TODO: this should be a void
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
xc_chat_view_set_time_stamp (XcChatView *xccv, gboolean showtimed)
{
  GtkTreeViewColumn *dtime;

  dtime = gtk_tree_view_get_column (xccv->tview, TVC_TIMED);
  gtk_tree_view_column_set_visible (dtime, showtimed);
}


void
//xc_chat_view_set_background (XcChatView *xccv, GdkPixmap *pixmap )
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
xc_chat_view_set_max_lines (XcChatView *xccv, int max_lines)
{
  xccv->max_lines = (gint) max_lines;
}
void
xc_chat_view_save (XcChatView *xccv, int fh)
{
  return;
}
void
xc_chat_view_copy_selection (XcChatView *xccv)
{
  GtkTreeModel	*model;
  GtkTreeIter	iter;
  GList		*rows, *r;
  GString	*hold;
  gchar		*h, *hh, *m, *nl,
		*newl = "\n",  *blank = "";
  gboolean	swap = TRUE;

  nl = blank;
  hold = g_string_new(NULL);
  rows = gtk_tree_selection_get_selected_rows (xccv->select, &model);

  for (r = rows; r != NULL; r = r->next)
  {
    if (gtk_tree_model_get_iter (model, &iter, r->data))
    {
      gtk_tree_model_get (model, &iter, SFS_HANDLE, &h, SFS_MESSAG, &m, -1);
      if (h)
        hh = h;
      else
        hh = blank;
      g_string_append_printf (hold, "%s<%s>\t%s", nl, hh, m);
      g_free(h);
      g_free(m);
      if (swap)
      {
        nl = newl;
        swap = FALSE;
      }
    }
  }
  g_list_free_full (rows, (GDestroyNotify) gtk_tree_path_free);
  gtk_clipboard_set_text (xccv->clippy, hold->str, hold->len);
  g_string_free (hold, TRUE);
}


void
xc_chat_view_load_scrollback (XcChatView *xccv, gchar *filen)
{
  
}

//int xc_chat_view_lastlog (xtext_buffer *out, xtext_buffer *search_area)
//{
//  return 1;
//}



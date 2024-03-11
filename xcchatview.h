/* HexChat
 * Copyright (C) 1998-2010 Peter Zelezny.
 * Copyright (C) 2009-2013 Berke Viktor.
 * Copyright (c) 2023-2024 somercet
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _xc_chat_view_h_
#define _xc_chat_view_h_

G_BEGIN_DECLS
#define XC_TYPE_CHAT_VIEW		(xc_chat_view_get_type ())
#define XC_CHAT_VIEW(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj),	XC_TYPE_CHAT_VIEW, XcChatView))
#define XC_CHAT_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass),	XC_TYPE_CHAT_VIEW, XcChatViewClass))
#define XC_IS_CHAT_VIEW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	XC_TYPE_CHAT_VIEW))
#define XC_IS_CHAT_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass),	XC_TYPE_CHAT_VIEW))
#define XC_CHAT_VIEW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),	XC_TYPE_CHAT_VIEW, XcChatViewClass))

typedef struct _XcChatView	XcChatView;
typedef struct _XcChatViewClass	XcChatViewClass;

#define ATTR_BOLD	'\002'
#define ATTR_COLOR	'\003'
#define ATTR_BLINK	'\006'
#define ATTR_BEEP	'\007'
#define ATTR_HIDDEN	'\010'
#define ATTR_ITALICS2	'\011'	// now monospace
#define ATTR_RESET	'\017'
#define ATTR_REVERSE	'\026'
#define ATTR_ITALICS	'\035'
#define ATTR_STRIKETHROUGH	'\036'
#define ATTR_UNDERLINE	'\037'

/* these match palette.h */
#define XTEXT_MIRC_COLS	32
#define XTEXT_COLS	37	/* 32 plus 5 for extra stuff below */
#define XTEXT_MARK_FG	32	/* for marking text */
#define XTEXT_MARK_BG	33
#define XTEXT_FG	34
#define XTEXT_BG	35
#define XTEXT_MARKER	36	/* for marker line */
#define XTEXT_MAX_COLOR	41

typedef enum marker_reset_reason_e {
	MARKER_WAS_NEVER_SET = 0,
	MARKER_IS_SET,
	MARKER_RESET_MANUALLY,
	MARKER_RESET_BY_KILL,
	MARKER_RESET_BY_CLEAR
} marker_reset_reason;

enum tvcolumns {
	TVC_TIMED = 0,
	TVC_HANDLE,
	TVC_MESSAGE,
	TVC_TOTAL
};

enum storefs {
	SFS_TMDATE = 0,
	SFS_HANDLE,
	SFS_MESSAG,
	SFS_GDTIME,
	SFS_COLNUM
};

struct _XcChatView
{
  /* Instance variables */
  /* Public  */
  GObject	parent;
  GtkTreeView	*tview;
  GtkListStore	*store;
  GtkClipboard	*clippy;
  //GtkWindow	*parent_widget;
  GtkWidget	*parent_widget;
  gulong	parent_reparent_cb_id;
  GtkAdjustment	*parent_vadj;

  GtkTreeSelection	*select;
  GtkCellRenderer	*cell_td, *cell_hn, *cell_ms;

  gchar		*dtformat;
  gchar		*scrollback_filename;

  guint		lines_max;
  guint		lines_current;
  gboolean	word_wrap;
  gint		word_wrap_width;
  gboolean	timestamps;

  gchar	*search_text;
  GList	*search_paths;
  GList	*search_current;
  GRegex	*search_regex;
  GtkWidget	*search_widget;
  GString	*search_label;
  guint	search_total;
  guint	search_now;
  gint	search_flags;

  GMutex	mutex;

  GtkTreeRowReference *marker_pos;
  marker_reset_reason marker_state;

  /* Private */
};

struct _XcChatViewClass
{
  GObjectClass	parent_class;

/* Standard OOP form function decs go here */
  void	(* append)	(XcChatView	*xccv,
			guchar		*message,
			gint		message_len,
			time_t		stamp );
  void (*append_indent)	(XcChatView	*xccv,
			guchar		*handle,
			gint		handle_len,
			guchar		*message,
			gint		message_len,
			time_t		stamp );
  void (* append0)	(XcChatView	*xccv,
			GDateTime	*dtime,
			gchar		*handle,
			gchar		*message);
  void (* prepend0)	(XcChatView	*xccv,
			GDateTime	*dtime,
			gchar		*handle,
			gchar		*message);
};


/*< Methods >*/

// GType gtk_xtext_get_type (void);
GType		xc_chat_view_get_type	(void ) G_GNUC_CONST;
// fkeys.c	key_dialog_show		xtext = gtk_xtext_new (colors, 0);
// maingui.c	mg_create_textarea	gui->xtext = gtk_xtext_new (colors, TRUE);
// rawlog.c	open_rawlog	serv->gui->rawlog_textlist = gtk_xtext_new (colors, 0);
// textgui.c	pevent_dialog_show
// GtkWidget *gtk_xtext_new (GdkColor palette[], int separator);
XcChatView *	xc_chat_view_new	(void );
// rawlog.c	fe_add_rawlog
// 	gtk_xtext_append (GTK_XTEXT (serv->gui->rawlog_textlist)->buffer, new_text, strlen (new_text), 0);
// textgui.c	PrintTextLine
//	if indent ; if prefs.hex_stamp_text
// void gtk_xtext_append (xtext_buffer *buf, unsigned char *text, int len, time_t stamp);
void		xc_chat_view_append (	XcChatView	*xccv,
					guchar		*message,
					int		message_len,
					time_t		stamp ); // gint64
// textgui.c	PrintTextLine
// void gtk_xtext_append_indent (xtext_buffer *buf, unsigned char *left_text, int left_len, unsigned char *right_text, int right_len, time_t stamp);
void	xc_chat_view_append_indent (	XcChatView	*xccv,
					guchar		*handle,
					int		handle_len,
					guchar		*message,
					int		message_len,
					time_t		stamp ); // gint64
// fe-gtk.c:	1. sess->res->buffer, lines 2. fe_text_clear
// fkeys.c:	key_dialog_print_text
// rawlog.c:	rawlog_clearbutton
// void gtk_xtext_clear (xtext_buffer *buf, int lines);
void	xc_chat_view_clear (	XcChatView	*xccv,
				gint		lines );
// fkeys.c	key_dialog_show
// maingui.c	mg_update_xtext
// rawlog.c	open_rawlog
// textgui.c	pevent_dialog_show
// int gtk_xtext_set_font (GtkXText *xtext, char *name);
int xc_chat_view_set_font (	XcChatView	*xccv,
				gchar		*name );
// maingui.c	mg_update_xtext < mg_create_textarea < mg_create_center < mg_create_irctab
// void gtk_xtext_set_wordwrap (GtkXText *xtext, gboolean word_wrap);
void xc_chat_view_set_wordwrap (	XcChatView	*xccv,
					gboolean	word_wrap );
// maingui.c	1. mg_add_chan 2. mg_create_topwindow 3. mg_apply_setup
// void gtk_xtext_set_time_stamp (xtext_buffer *buf, gboolean timestamp);
void xc_chat_view_set_time_stamp (	XcChatView	*xccv,
					gboolean	timestamp );
// maingui.c	1. mg_update_xtext < mg_create_textarea
// void gtk_xtext_set_background (GtkXText * xtext, GdkPixmap * pixmap);
void xc_chat_view_set_background (	XcChatView	*xccv,
					gchar		*file );
// void gtk_xtext_set_palette (GtkXText * xtext, GdkColor palette[]);
void xc_chat_view_set_palette (		XcChatView	*xccv,
//#ifdef GTK3
//					GdkRGBA		palette[] );
//#else
					GdkColor	palette[] );
//#endif
// menu.c	menu_resetmarker
// void gtk_xtext_reset_marker_pos (GtkXText *xtext);
void xc_chat_view_reset_marker_pos (	XcChatView	*xccv );
// menu.c	menu_movetomarker
// int gtk_xtext_moveto_marker_pos (GtkXText *xtext);
int xc_chat_view_moveto_marker_pos (	XcChatView	*xccv );
// maingui.c	1. mg_tabwin_focus_cb 2. mg_topwin_focus_cb
// void gtk_xtext_check_marker_visibility (GtkXText *xtext);
void xc_chat_view_check_marker_visibility (	XcChatView	*xccv );
// maingui.c	mg_create_textarea
// void gtk_xtext_set_show_marker (GtkXText *xtext, gboolean show_marker);
void xc_chat_view_set_show_marker (	XcChatView	*xccv,
					gboolean	show_marker );
// void gtk_xtext_set_show_separator (GtkXText *xtext, gboolean show_separator);
void xc_chat_view_set_show_separator (	XcChatView	*xccv,
					gboolean	show_separator );
// void gtk_xtext_set_thin_separator (GtkXText *xtext, gboolean thin_separator);
void xc_chat_view_set_thin_separator (	XcChatView	*xccv,
					gboolean	thin_separator );
// void gtk_xtext_set_indent (GtkXText *xtext, gboolean indent);
void xc_chat_view_set_indent (		XcChatView	*xccv,
					gboolean	indent );
// void gtk_xtext_set_max_indent (GtkXText *xtext, int max_auto_indent);
void xc_chat_view_set_max_indent (	XcChatView	*xccv,
					int		max_auto_indent );
// maingui.c	1. mg_update_xtext 2. mg_create_textarea
// void gtk_xtext_set_max_lines (GtkXText *xtext, int max_lines);
void xc_chat_view_set_max_lines (	XcChatView	*xccv,
					int		max_lines );
// menu.c	menu_copy_selection
// gtk_xtext_copy_selection (GTK_XTEXT (current_sess->gui->xtext));
// void gtk_xtext_copy_selection (GtkXText *xtext);
void xc_chat_view_copy_selection (	XcChatView	*xccv );
// menu.c	menu_savebuffer
// rawlog.c	rawlog_save
// void gtk_xtext_save (GtkXText * xtext, int fh);
void xc_chat_view_save (	XcChatView	*xccv,
				int		fh );
// maingui.c	search_handle_event
// textentry *gtk_xtext_search (GtkXText * xtext, const gchar *text, gtk_xtext_search_flags flags, GError **err);
void xc_chat_view_run_search (	XcChatView	*xccv,
				const gchar	*search_text,
				gint		flags,
				GError		**serr );

void xc_chat_view_next_search (	XcChatView	*xccv,
				gboolean	direction );

void xc_chat_view_lastlog (	XcChatView	*xccv,
				XcChatView	*target,
				const gchar	*text,
				gint		flags );

/*
hexchat/src/common/outbound.c
static void lastlog (session *sess, char *search, gtk_xtext_search_flags flags)

// fe-gtk.c	fe_lastlog
//int gtk_xtext_lastlog (xtext_buffer *out, xtext_buffer *search_area);
*/



void xc_chat_view_set_scrollback_file (	XcChatView *xccv,
					gchar *filename );

void		xc_chat_view_prepend0 (	XcChatView	*xccv,
					GDateTime	*dtime,
					gchar		*handle,
					gchar		*message );

void		xc_chat_view_append0 (	XcChatView	*xccv,
					GDateTime	*dtime,
					gchar		*handle,
					gchar		*message );

// comment out for now
//
// maingui.c	mg_create_textarea
// gtk_xtext_set_urlcheck_function (xtext, mg_word_check);
// void gtk_xtext_set_urlcheck_function (GtkXText *xtext, int (*urlcheck_function) (GtkWidget *, char *));


// TODO: this one is a doozy.  It looks as though setting up
// proper signal/slot handling for marker should render it moot.
// fe-gtk.c	fe_new_window
// sess->scrollback_replay_marklast = gtk_xtext_set_marker_last;
// void gtk_xtext_set_marker_last (	session		*sess);
// scrollback: gtk_cell_renderer_set_visible()
void	xc_chat_view_set_marker_last (	gpointer sessresbuf );

void	xc_chat_view_push_down_scrollbar (	XcChatView	*xccv );


// TODO: do not replace these, comment out

// maingui.c	mg_update_xtext
// void gtk_xtext_refresh (GtkXText * xtext);

// maingui.c	xtext size change? Then don't render, wait for the expose caused
//		by showing/hidding the userlist
//	gtk_xtext_buffer_show (GTK_XTEXT (gui->xtext), res->buffer, bool render)
// void gtk_xtext_buffer_show (GtkXText *xtext, xtext_buffer *buf, int render);

// xtext.c
// typedef void (*GtkXTextForeach) (GtkXText *xtext, unsigned char *text, void *data);
// void gtk_xtext_foreach (xtext_buffer *buf, GtkXTextForeach func, void *data);

// none
// void gtk_xtext_set_error_function (GtkXText *xtext, void (*error_function) (int));

// fe-gtk.c	fe_lastlog search
// gboolean gtk_xtext_is_empty (xtext_buffer *buf);


// maingui.c	fe_session_callback, called to kill session
// void gtk_xtext_buffer_free (xtext_buffer *buf);


G_END_DECLS
#endif /* _xc_chat_view_h_ */


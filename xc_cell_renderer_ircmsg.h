#ifndef _xc_cell_renderer_ircmsg_h_
#define _xc_cell_renderer_ircmsg_h_

G_BEGIN_DECLS

typedef struct
{
	GtkCellRendererText parent;
	gchar	*irctext;
} XcCellRendererIrcmsg;

typedef struct
{
	GtkCellRendererTextClass parent_class;
} XcCellRendererIrcmsgClass;

#define XC_TYPE_CELL_RENDERER_IRCMSG (xc_cell_renderer_ircmsg_get_type ())
#define XC_CELL_RENDERER_IRCMSG(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), XC_TYPE_CELL_RENDERER_IRCMSG, XcCellRendererIrcmsg))
#define XC_CELL_RENDERER_IRCMSG_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), XC_CELL_RENDERER_IRCMSG, XcCellRendererIrcmsgClass))
#define XC_IS_CELL_RENDERER_IRCMSG(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XC_TYPE_CELL_RENDERER_IRCMSG))
#define XC_IS_CELL_RENDERER_IRCMSG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XC_TYPE_CELL_RENDERER_IRCMSG))
#define XC_CELL_RENDERER_IRCMSG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), XC_TYPE_CELL_RENDERER_IRCMSG, XcCellRendererIrcmsgClass))

GtkCellRenderer * xc_cell_renderer_ircmsg_new (void);

G_END_DECLS

#endif // _xc_cell_renderer_ircmsg_h_

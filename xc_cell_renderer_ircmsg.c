#ifndef __GTK_H__
#include <gtk/gtk.h>
#endif
#ifndef _xc_cell_renderer_ircmsg_h_
#include "xc_cell_renderer_ircmsg.h"
#endif

enum  { 
	PROP_0,
	PROP_IRCTEXT,
	LAST_PROP
};

static GParamSpec* xc_cell_renderer_ircmsg_properties[LAST_PROP];

G_DEFINE_TYPE (XcCellRendererIrcmsg, xc_cell_renderer_ircmsg, GTK_TYPE_CELL_RENDERER_TEXT)

static PangoAttrList *
xc_parse (const char *text)
{
	PangoAttrList *attrs = pango_attr_list_new ();
	gboolean red = FALSE;
	gint i = 0;

	while (text[i] != '\0')
	{
		if (text[i] == 'a')
			red = !red;

		PangoAttribute *attr = pango_attr_foreground_new (red ? 65535 : 0, 0, 0);
		attr->start_index = i;
		attr->end_index = i + 1;
		pango_attr_list_insert (attrs, attr);
		i++;
	}

	return attrs;
}

static void
xc_cell_renderer_ircmsg_get_property (GObject *object, guint prop_id, GValue *value,
													 GParamSpec *pspec)
{
//	XcCellRendererIrcmsg *cell = (XcCellRendererIrcmsg *) object;

	switch (prop_id) {
	case PROP_IRCTEXT:
		g_value_set_string (value, NULL);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xc_cell_renderer_ircmsg_set_property (GObject *object, guint prop_id, const GValue *value,
													 GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_IRCTEXT:
//		gchar *str = NULL;
//		PangoAttrList *attrs = NULL;
		const gchar *markup = g_value_get_string (value);
//		xc_parse (markup, str, attrs);
		PangoAttrList *attrs = xc_parse (markup);
		g_object_freeze_notify (object);
		g_object_set (object, "text", markup, NULL);
		g_object_set (object, "attributes", attrs, NULL);
		g_object_thaw_notify (object);
		pango_attr_list_unref (attrs);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xc_cell_renderer_ircmsg_class_init (XcCellRendererIrcmsgClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->get_property = xc_cell_renderer_ircmsg_get_property;
	gobject_class->set_property = xc_cell_renderer_ircmsg_set_property;

	g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_IRCTEXT,
		xc_cell_renderer_ircmsg_properties[PROP_IRCTEXT] =
			g_param_spec_string ("irctext", "IRC message text", "IRC message text with embedded formatting codes",
				NULL, G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE));
}

static void
xc_cell_renderer_ircmsg_init (XcCellRendererIrcmsg *self)
{
	return;
}

GtkCellRenderer *
xc_cell_renderer_ircmsg_new (void)
{
	XcCellRendererIrcmsg *self = g_object_new (XC_TYPE_CELL_RENDERER_IRCMSG, NULL);
	return GTK_CELL_RENDERER (self);

}


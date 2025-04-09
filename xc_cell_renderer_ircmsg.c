#ifndef __GTK_H__
#include <gtk/gtk.h>
#endif
#ifndef _xc_cell_renderer_ircmsg_h_
#include "xc_cell_renderer_ircmsg.h"
#endif

enum
{
	PROP_0,
	PROP_IRCTEXT,
	LAST_PROP
};

enum {
	BOLD = 1,
	ITAL = 2,
	SLSH = 4,
	ULIN = 8,
	MONO = 16,
	REVR = 32,
	COLR = 64,
	HCOL = 128,
	HIDD = 256,
	BEEP = 512
};


static GParamSpec * xc_cell_renderer_ircmsg_properties[LAST_PROP];

G_DEFINE_TYPE (XcCellRendererIrcmsg, xc_cell_renderer_ircmsg, GTK_TYPE_CELL_RENDERER_TEXT);

#define ATTR_BOLD       '\002' // STX
#define ATTR_COLOR      '\003' // ETX
#define ATTR_RESET      '\017' // SI  0x0f
#define ATTR_MONOSPACE  '\021' // DC1 0x11
#define ATTR_REVERSE    '\026' // SYN 0x16
#define ATTR_ITALIC     '\035' // GS  0x1d
#define ATTR_SLASH      '\036' // RS  0x1e STRIKETHROUGH
#define ATTR_UNDERLINE  '\037' // US  0x1f 

/*
	code |= BOLD;							// on
	code &= ~BOLD;							// off
	if (code & BOLD)						// true if set
	if (code & (BOLD | ITAL)		)	// true if both set
	if ((code & (BOLD | ITAL)) == 0)	// true if both unset

> n = n & 0177; 
> sets to zero all but the low-order 7 bits of n. 

#define CHAR_BOLD		'\002'
#define TEST_BOLD		001  // 00000001 to turn ON
#define MASK_BOLD		0xfe // 11111110 to turn OFF **OR** use ~001

*/

static void
switch_code (int code, int mask)
{
	if (code & mask)
		code &= ~mask;
	else
		code |= mask;
}


static void
xc_parse (gchar *src, gchar *dest, PangoAttrList *attrs)
{
	gshort s = 0, d = 0;
	gint code = 0;
	gchar tmp[1024];
	enum { START, BODY, CODES };
	gint stat = START;

	while (*src)
	{
		switch (*src) {
		case ATTR_BOLD:
			if (stat == BODY && code)
				close_attr (code, d);
			switch_code (code, BOLD);
			stat = CODES;
			break;
		case ATTR_ITALIC:
			if (stat == BODY && code)
				close_attr (code, d);
			switch_code (code, ITALIC);
			stat = CODES;
			break;
		case ATTR_RESET:
			if (stat == BODY && code)
				close_attr (code, d);
			code = 0;
			stat = CODES;
			break;
		default:						  // all non-control chars
			if (stat == CODES && code)
				close_attr (code, d);
			tmp[d++] = *src++;
			stat = BODY;
			break;
		}

	}

}



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
// XcCellRendererIrcmsg *cell = (XcCellRendererIrcmsg *) object;

	switch (prop_id)
	{
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
	switch (prop_id)
	{
	case PROP_IRCTEXT:
//    gchar *str = NULL;
//    PangoAttrList *attrs = NULL;
		const gchar *markup = g_value_get_string (value);
//    xc_parse (markup, str, attrs);
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
												g_param_spec_string ("irctext", "IRC message text",
																			"IRC message text with embedded formatting codes",
																			NULL,
																			G_PARAM_STATIC_STRINGS|G_PARAM_READWRITE));
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


/*

#define IRC_BOLD      '\002'
#define IRC_COLOR     '\003'
#define IRC_RESET     '\017'
#define IRC_ITALIC    '\026'
#define IRC_UNDERLINE '\037'

typedef struct
{
	gboolean bold;
	gboolean italic;
	gboolean underline;
	int fg_color;
	gboolean has_fg;
} IrcStyle;

void
parse_irc_to_pango (const char *input, char **out_clean_text, PangoAttrList **out_attrs)
{
	GString *clean = g_string_sized_new (512);
	PangoAttrList *attrs = pango_attr_list_new ();

	IrcStyle style = { 0 };
	int input_pos = 0;			  // Position in original text
	int output_pos = 0;			  // Position in clean text

	int segment_start = 0;
	IrcStyle prev_style = { 0 };

	while (input[input_pos])
	{
		char ch = input[input_pos];

		gboolean control = TRUE;

		select (ch) {
		case IRC_BOLD:
			style.bold = !style.bold;
			break;
		case IRC_ITALIC:
			style.italic = !style.italic;
			break;
		case IRC_UNDERLINE:
			style.underline = !style.underline;
			break;
		case IRC_COLOR:
			input_pos++;
			style.has_fg = FALSE;
			style.fg_color = 0;

			if (g_ascii_isdigit (input[input_pos]))
			{
				int fg = g_ascii_digit_value (input[input_pos]);
				input_pos++;
				if (g_ascii_isdigit (input[input_pos]))
				{
					fg = fg * 10 + g_ascii_digit_value (input[input_pos]);
					input_pos++;
				}
				style.has_fg = TRUE;
				style.fg_color = fg;
			} else
				style.has_fg = FALSE; // Reset color

			input_pos--;				// we'll re-increment below
			break;
		case IRC_RESET:
			style = (IrcStyle) {0};
			break;
		default: 						// Regular character
			g_string_append_c (clean, ch);
			output_pos++;
			control = FALSE;
			break;
		}

		if (ch == IRC_BOLD)
		{
			style.bold = !style.bold;
		} else if (ch == IRC_ITALIC)
		{
			style.italic = !style.italic;
		} else if (ch == IRC_UNDERLINE)
		{
			style.underline = !style.underline;
		} else if (ch == IRC_COLOR)
		{
			input_pos++;
			style.has_fg = FALSE;
			style.fg_color = 0;

			if (g_ascii_isdigit (input[input_pos]))
			{
				int fg = g_ascii_digit_value (input[input_pos]);
				input_pos++;
				if (g_ascii_isdigit (input[input_pos]))
				{
					fg = fg * 10 + g_ascii_digit_value (input[input_pos]);
					input_pos++;
				}
				style.has_fg = TRUE;
				style.fg_color = fg;
			} else
			{
				// Reset color
				style.has_fg = FALSE;
			}
			input_pos--;			  // we'll re-increment below
		} else if (ch == IRC_RESET)
		{
			style = (IrcStyle) {0};
		} else
		{
			// Regular character
			g_string_append_c (clean, ch);
			output_pos++;
			control = FALSE;
		}

		if (memcmp (&style, &prev_style, sizeof (IrcStyle)) != 0)
		{
			// Style changed → close previous span
			if (segment_start != output_pos)
			{
				if (prev_style.bold)
				{
					PangoAttribute *a = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
					a->start_index = segment_start;
					a->end_index = output_pos;
					pango_attr_list_insert (attrs, a);
				}
				if (prev_style.italic)
				{
					PangoAttribute *a = pango_attr_style_new (PANGO_STYLE_ITALIC);
					a->start_index = segment_start;
					a->end_index = output_pos;
					pango_attr_list_insert (attrs, a);
				}
				if (prev_style.underline)
				{
					PangoAttribute *a = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
					a->start_index = segment_start;
					a->end_index = output_pos;
					pango_attr_list_insert (attrs, a);
				}
				if (prev_style.has_fg)
				{
					// IRC colors are 0–15; you may want to map them to real GDK colors
					static const char *color_table[] = {
						"#FFFFFF", "#000000", "#00007F", "#009300", "#FF0000",
						"#7F0000", "#9C009C", "#FC7F00", "#FFFF00", "#00FC00",
						"#009393", "#00FFFF", "#0000FC", "#FF00FF", "#7F7F7F", "#D2D2D2"
					};
					if (prev_style.fg_color >= 0 && prev_style.fg_color < 16)
					{
						PangoAttribute *a =
							pango_attr_foreground_new (gdk_rgba_parse
																((GdkRGBA[]){ 0 },
																 color_table[prev_style.
																				 fg_color]) ? ((GdkRGBA[]){ 0 }
																)->red * 65535 : 0,
																((GdkRGBA[]){ 0 }
																)->green * 65535,
																((GdkRGBA[]){ 0 }
																)->blue * 65535);
						a->start_index = segment_start;
						a->end_index = output_pos;
						pango_attr_list_insert (attrs, a);
					}
				}
			}
			// Start new span
			prev_style = style;
			segment_start = output_pos;
		}

		input_pos++;
	}

	// Handle final segment
	if (segment_start != output_pos)
	{
		if (style.bold)
		{
			PangoAttribute *a = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
			a->start_index = segment_start;
			a->end_index = output_pos;
			pango_attr_list_insert (attrs, a);
		}
		if (style.italic)
		{
			PangoAttribute *a = pango_attr_style_new (PANGO_STYLE_ITALIC);
			a->start_index = segment_start;
			a->end_index = output_pos;
			pango_attr_list_insert (attrs, a);
		}
		if (style.underline)
		{
			PangoAttribute *a = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
			a->start_index = segment_start;
			a->end_index = output_pos;
			pango_attr_list_insert (attrs, a);
		}
		if (style.has_fg && style.fg_color >= 0 && style.fg_color < 16)
		{
			// same color logic here if needed
		}
	}

	*out_clean_text = g_string_free (clean, FALSE);
	*out_attrs = attrs;
}
*/


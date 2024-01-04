/*
gcc -g -Wall $(pkgconf --cflags gio-2.0) -o atac atac.c $(pkgconf --libs gio-2.0)
*/

#include <glib.h>
#include <gio/gio.h>

GMainLoop *Loop;

static void
file_read_complete (GObject *file, GAsyncResult *res, gpointer user_data) {
	GError *error = NULL;
	char *contents;
	gsize length;

	if (g_file_load_contents_finish (G_FILE (file), res, &contents, &length, NULL, &error)) { // etag
		gchar **lines = g_strsplit (contents, "\n", 0);
		GTimeZone *tz = g_time_zone_new_local ();
		gchar *empty = "";

		for (guint i = g_strv_length (lines) ; i > 1; i--) {
			gchar *t = NULL, *h = NULL, *m = NULL;

			if (! strlen (lines[i - 2])) {
				g_print ("\t\t\n");
				continue;
			}
			gchar **f = g_strsplit (lines[i - 2], "\t", 3);
			g_print ("%d\n", g_strv_length (f));
			if (! strlen (f[0]))
				t = empty;
			else {
				GDateTime *dt = g_date_time_new_from_iso8601 (f[0], NULL);

				if (dt) {
					GDateTime *nw = g_date_time_to_timezone (dt, tz);

					t = g_date_time_format_iso8601 (nw);

					g_date_time_unref(dt);
					g_date_time_unref(nw);
				} else
					t = empty;
			}
			if (f[1] != NULL)
				h = f[1];
			else
				h = empty;
			if (f[2] != NULL)
				m = f[2];
			else
				m = empty;

			g_print ("%s\t%s\t%s\n", t, h, m);

			g_strfreev (f);
			if (t != empty && t != NULL)
				g_free (t);
		}

		g_strfreev (lines);
		g_free (contents);
		g_time_zone_unref(tz);
	} else {
		g_printerr ("Error reading file: %s\n", error->message);
		g_error_free (error);
	}

	g_object_unref (file);
	g_main_loop_quit (Loop);
	g_main_loop_unref (Loop);
}


static void
async_file_read (const gchar *filename) {
	GFile *file = g_file_new_for_path (filename);

	g_file_load_contents_async (file, NULL, file_read_complete, NULL); // cancellable, user data

	Loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (Loop);
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		g_printerr ("Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	const gchar *filename = argv[1];
	async_file_read (filename);

	return 0;
}



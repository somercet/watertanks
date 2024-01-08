/*
gcc -g -Wall $(pkgconf --cflags gio-2.0) -o atac atac.c $(pkgconf --libs gio-2.0)
*/

#include <glib.h>
#include <gio/gio.h>


static void
file_read_complete (GObject *file, GAsyncResult *res, gpointer loop) {
	GError *error = NULL;
	char *contents;
	gsize length;

	if (g_file_load_contents_finish (G_FILE (file), res, &contents, &length, NULL, &error)) { // etag
		gchar **lines = g_strsplit (contents, "\n", 0);
		GTimeZone *tz = g_time_zone_new_local ();
		gchar *empty = "";

		for (guint i = g_strv_length (lines) ; i > 1; i--) {
			gchar *t = NULL;
			gchar **f = g_strsplit (lines[i - 1], "\t", 3);

			if (f == NULL || g_strv_length (f) < 3) {
				g_print ("\t\t\n");
				g_strfreev (f);
				continue;
			} else {
				GDateTime *dt = g_date_time_new_from_iso8601 (f[0], NULL);

				if (dt) {
					GDateTime *nw = g_date_time_to_timezone (dt, tz);

					t = g_date_time_format_iso8601 (nw);

					g_date_time_unref(dt);
					g_date_time_unref(nw);
				} else
					t = empty;
			}

			g_print ("%s\t%s\t%s\n", t, f[1], f[2]);

			if (f)
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
	g_main_loop_quit (loop);
}


static void
async_file_read (const gchar *filename, GMainLoop *loop) {
	GFile *file = g_file_new_for_path (filename);
	loop = g_main_loop_new (NULL, FALSE);

	g_file_load_contents_async (file, NULL, file_read_complete, loop); // cancellable
	g_main_loop_run (loop);
}

int main(int argc, char *argv[]) {
	GMainLoop *loop = g_main_loop_new (NULL, FALSE);

	if (argc < 2) {
		g_printerr ("Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	const gchar *filename = argv[1];
	async_file_read (filename, loop);
	g_main_loop_unref (loop);

	return 0;
}



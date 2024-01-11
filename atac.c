/*
gcc -g -Wall $(pkgconf --cflags gio-2.0) -o atac atac.c $(pkgconf --libs gio-2.0)
*/

#include <glib.h>
#include <gio/gio.h>


static void
load_scrollback_finish (GObject *sobj, GAsyncResult *res, gpointer loop) {
	GTask *task = G_TASK(res);
	GError *error = NULL;

	if (! g_task_propagate_boolean (task, &error)) {
		g_printerr ("Error reading file: %s\n", error->message);
		g_error_free (error);
	}

	g_object_unref(task);
	g_main_loop_quit (loop);
}


static void
load_scrollback_run (GTask *task, gpointer sobj, gpointer loop, GCancellable *cancellable) {
	GFile *file = G_FILE (sobj);
	GError *error = NULL;
	char *contents;
	gsize length;
	gboolean ret;

	if (g_file_load_contents (file, NULL, &contents, &length, NULL, &error)) // cancellable, etag
	{
		gchar **lines = g_strsplit (contents, "\n", 0);
		GTimeZone *tz = g_time_zone_new_local ();
		gchar *empty = "";

		for (guint i = g_strv_length (lines) ; i > 0; i--) {
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
		ret = TRUE;
	} else {
		g_printerr ("Error reading file: %s\n", error->message);
		g_error_free (error);
		ret = FALSE;
	}

	g_task_return_boolean (task, ret);
	g_object_unref (file);
}


static void
load_scrollback_start (const gchar *filename, GMainLoop *loop) {
	GFile *file = g_file_new_for_path (filename);
	GTask *task = g_task_new (file, NULL, load_scrollback_finish, loop); // cancellable

	g_task_run_in_thread (task, load_scrollback_run);
	g_main_loop_run (loop);
}


int main(int argc, char *argv[]) {
	GMainLoop *loop = g_main_loop_new (NULL, FALSE);

	if (argc < 2) {
		g_printerr ("Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	const gchar *filename = argv[1];
	load_scrollback_start (filename, loop);
	g_main_loop_unref (loop);

	return 0;
}



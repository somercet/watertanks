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
		gchar **lines = g_strsplit (contents, "\n", -1);

		for (guint i = g_strv_length (lines) ; i > 1; i--)
			g_print ("%s\n", lines[i - 2]);

		g_strfreev (lines);
		g_free (contents);
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



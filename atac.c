/*
gcc -g -Wall $(pkgconf --cflags gio-2.0) -o atac atac.c $(pkgconf --libs gio-2.0)
*/

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

typedef struct {
    gchar **lines;
    gint num_lines;
} FileData;


static void file_read_complete(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GError *error = NULL;
    FileData *file_data = (FileData *)user_data;

    gssize bytes_read = g_input_stream_read_finish(G_INPUT_STREAM(source_object), res, &error);

    if (bytes_read > 0) {
        file_data->lines = g_strsplit(file_data->lines[0], "\n", -1);
        file_data->num_lines = g_strv_length(file_data->lines);

        for (gint i = file_data->num_lines - 1; i >= 0; i--) {
            if (file_data->lines[i] != NULL && file_data->lines[i][0] != '\0') {
                g_print("%s\n", file_data->lines[i]);
            }
        }

        g_strfreev(file_data->lines);
        g_free(file_data);
    } else if (bytes_read == 0) {
        // End of file
        g_strfreev(file_data->lines);
        g_free(file_data);
    } else {
        g_printerr("Error reading file: %s\n", error->message);
        g_error_free(error);
        g_strfreev(file_data->lines);
        g_free(file_data);
    }

    g_main_loop_quit(g_main_loop_new(NULL, FALSE));
}


static void async_file_read(const gchar *filename) {
    GFileInputStream *input_stream;
    GFile *file = g_file_new_for_path(filename);

    input_stream = g_file_read(file, NULL, NULL);

    FileData *file_data = g_new(FileData, 1);
    file_data->lines = NULL;
    file_data->num_lines = 0;

    g_input_stream_read_async(
        G_INPUT_STREAM(input_stream),
        g_malloc0(4096),  // Buffer size (adjust as needed)
        4096,             // Buffer size (adjust as needed)
        G_PRIORITY_DEFAULT,
        NULL,
        file_read_complete,
        file_data);

    g_object_unref(file);
    g_object_unref(input_stream);

    g_main_loop_run(g_main_loop_new(NULL, FALSE));
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        g_printerr("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    const gchar *filename = argv[1];
    async_file_read(filename);

    return 0;
}



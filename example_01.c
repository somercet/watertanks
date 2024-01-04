
#include <gtk/gtk.h>


enum {
  LS_TMDT,
  LS_HNDL,
  LS_MSSG,
  LS_TIME,
  LS_NCLM
};


static void
dtime_cell_dfunc (GtkTreeViewColumn *tree_column,
		  GtkCellRenderer *cell,
		  GtkTreeModel *tree_model,
		  GtkTreeIter *iter,
		  gpointer data)
{
  GDateTime *dtime;
  gchar *result;

  gtk_tree_model_get (tree_model, iter, LS_TIME, &dtime, -1);
  result = g_date_time_format (dtime, "%FT%T");
  
  g_object_set(cell, "text", result, NULL);
  g_date_time_unref (dtime);
  g_free (result);
}


static void
activate (GtkApplication* app,
          gpointer        user_data)
{
  GtkWidget *window;
  GtkTreeView *cv;
  GtkListStore *list_store;
  GtkTreeIter iter;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *cell;//, *cellw
  GDateTime *now;
  //GtkMultiSelection *multisel;
  //multisel = gtk_multi_selection_new ();

  list_store = gtk_list_store_new (LS_NCLM,
	G_TYPE_STRING,
	G_TYPE_STRING,
	G_TYPE_STRING,
	G_TYPE_DATE_TIME);

  now = g_date_time_new_now_local ();
  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (list_store, &iter,
    LS_TMDT, "foo",
    LS_HNDL, "bar",
    LS_MSSG, "baz",
    LS_TIME, now,
    -1);

  window = gtk_application_window_new (app);
  cv = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store)));
  g_object_unref (list_store);
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (cv));
  gtk_window_set_title (GTK_WINDOW (window), "Window");
  gtk_window_set_default_size (GTK_WINDOW (window), 200, 200);

  gtk_tree_view_set_headers_visible (cv, FALSE);
  cell = gtk_cell_renderer_text_new ();
/*  cellw = gtk_cell_renderer_text_new ();
  g_object_set (cellw,
	"wrap-mode", PANGO_WRAP_WORD_CHAR,
	"wrap-width", 0,
	NULL);*/

//  gtk_tree_view_insert_column_with_attributes (cv, -1, "Date", cell, "text", LS_TMDT, NULL);
  gtk_tree_view_insert_column_with_data_func (cv, -1, "Date", cell, dtime_cell_dfunc, NULL, NULL);
  gtk_tree_view_insert_column_with_attributes (cv, -1, "Handle", cell, "text", LS_HNDL, NULL);
  gtk_tree_view_insert_column_with_attributes (cv, -1, "Messages", cell, "text", LS_MSSG, NULL);

  gtk_widget_show_all (window);
}


int
main (int    argc,
      char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}


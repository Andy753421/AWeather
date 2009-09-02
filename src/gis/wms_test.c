#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "wms.h"

/***********
 * Testing *
 ***********/
GtkWidget *image  = NULL;
GtkWidget *bar    = NULL;

void done_callback(WmsCacheNode *node, gpointer user_data)
{
	g_message("done_callback: %p->%p", node, node->data);
	gtk_image_set_from_pixbuf(GTK_IMAGE(image), node->data);
}

void chunk_callback(gsize cur, gsize total, gpointer user_data)
{
	g_message("chunk_callback: %d/%d", cur, total);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(bar), (gdouble)cur/total);
}

gboolean key_press_cb(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	if (event->keyval == GDK_q)
		gtk_main_quit();
	return TRUE;
}

int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);
	g_thread_init(NULL);

	GtkWidget *win    = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget *box    = gtk_vbox_new(FALSE, 0);
	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	image = gtk_image_new();
	bar   = gtk_progress_bar_new();
	gtk_container_add(GTK_CONTAINER(win), box);
	gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), image);
	gtk_box_pack_start(GTK_BOX(box), bar, FALSE, FALSE, 0);
	g_signal_connect(win, "key-press-event", G_CALLBACK(key_press_cb), NULL);
	g_signal_connect(win, "destroy", gtk_main_quit, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gdouble res = 200, lon = -121.76, lat = 46.85;

	WmsInfo *info = wms_info_new_for_bmng(bmng_pixbuf_loader, bmng_pixbuf_freeer);
	wms_info_cache(info, res, lat, lon, NULL, NULL, NULL);
	WmsCacheNode *node = wms_info_fetch(info, res, lat, lon, NULL);
	if (node) gtk_image_set_from_pixbuf(GTK_IMAGE(image), node->data);
	wms_info_cache(info, res, lat, lon, NULL,           done_callback, NULL);
	wms_info_cache(info, res, lat, lon, chunk_callback, done_callback, NULL);

	//WmsInfo *info = wms_info_new_for_srtm(srtm_pixbuf_loader, srtm_pixbuf_freeer);
	//wms_info_cache(info, res, lat, lon, chunk_callback, NULL, NULL);
	//WmsCacheNode *node = wms_info_fetch(info, res, lat, lon, NULL);
	//if (node) gtk_image_set_from_pixbuf(GTK_IMAGE(image), node->data);

	gtk_widget_show_all(win);
	gtk_main();

	wms_info_free(info);

	return 0;
}

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "wms.h"

struct CacheState {
	GtkWidget *image;
	GtkWidget *status;
	GtkWidget *progress;
};

void done_callback(WmsCacheNode *node, gpointer _state)
{
	struct CacheState *state = _state;
	g_message("done_callback: %p->%p", node, node->data);
	gtk_image_set_from_pixbuf(GTK_IMAGE(state->image), node->data);
}

void chunk_callback(gsize cur, gsize total, gpointer _state)
{
	struct CacheState *state = _state;
	g_message("chunk_callback: %d/%d", cur, total);

	if (state->progress == NULL) {
		state->progress = gtk_progress_bar_new();
		gtk_box_pack_end(GTK_BOX(state->status), state->progress, FALSE, FALSE, 0);
		gtk_widget_show(state->progress);
	}

	if (cur == total)
		gtk_widget_destroy(state->progress);
	else
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(state->progress), (gdouble)cur/total);
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

	GtkWidget *win        = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget *vbox1      = gtk_vbox_new(FALSE, 0);
	GtkWidget *vbox2      = gtk_vbox_new(FALSE, 0);
	GtkWidget *status     = gtk_statusbar_new();
	GtkWidget *scroll     = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *bmng_image = gtk_image_new();
	GtkWidget *srtm_image = gtk_image_new();
	gtk_container_add(GTK_CONTAINER(win), vbox1);
	gtk_box_pack_start(GTK_BOX(vbox1), scroll, TRUE, TRUE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), vbox2);
	gtk_box_pack_start(GTK_BOX(vbox2), bmng_image, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), srtm_image, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), status, FALSE, FALSE, 0);
	g_signal_connect(win, "key-press-event", G_CALLBACK(key_press_cb), NULL);
	g_signal_connect(win, "destroy", gtk_main_quit, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	struct CacheState bmng_state = {bmng_image, status, NULL};
	struct CacheState srtm_state = {srtm_image, status, NULL};

	gdouble res = 200, lon = -101.76, lat = 46.85;

	WmsInfo *bmng_info = wms_info_new_for_bmng(bmng_pixbuf_loader, bmng_pixbuf_freeer);
	wms_info_fetch_cache(bmng_info, NULL, res, lat, lon, chunk_callback, done_callback, &bmng_state);

	WmsInfo *srtm_info = wms_info_new_for_srtm(srtm_pixbuf_loader, srtm_pixbuf_freeer);
	wms_info_fetch_cache(srtm_info, NULL, res, lat, lon, chunk_callback, done_callback, &srtm_state);

	gtk_widget_show_all(win);
	gtk_main();

	wms_info_free(bmng_info);
	wms_info_free(srtm_info);

	return 0;
}

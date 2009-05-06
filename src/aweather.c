#include <config.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkkeysyms.h>

#include "opengl.h"
#include "radar.h"
#include "ridge.h"
#include "example.h"

static void destroy(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	destroy(widget, data);
	return FALSE;
}

static gboolean key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (event->keyval == GDK_q)
		destroy(widget, data);
	return TRUE;
}

int main(int argc, char *argv[])
{

	gtk_init(&argc, &argv);
	gtk_gl_init(&argc, &argv);

	/* Set up window */
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	g_signal_connect(G_OBJECT(window), "delete-event",    G_CALLBACK(delete_event), NULL);
	g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(key_press),    NULL);

	/* Set up layout */
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	GtkWidget *paned = gtk_vpaned_new();
	gtk_box_pack_end(GTK_BOX(vbox), paned, TRUE, TRUE, 0);

	/* Set up menu bar */
	GtkWidget *menu                = gtk_menu_bar_new();
	GtkWidget *menu_file           = gtk_menu_item_new_with_label("File");
	GtkWidget *menu_file_menu      = gtk_menu_new();
	GtkWidget *menu_file_menu_quit = gtk_menu_item_new_with_label("Quit");
	gtk_box_pack_start(GTK_BOX(vbox), menu, FALSE, FALSE, 0);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_file);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_file), menu_file_menu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_file_menu), menu_file_menu_quit);
	g_signal_connect(G_OBJECT(menu_file_menu_quit), "activate", G_CALLBACK(destroy), NULL);

	/* Set up darwing area */
	GtkWidget *drawing = gtk_drawing_area_new();
	gtk_paned_pack1(GTK_PANED(paned), drawing, TRUE, FALSE);
	//gtk_box_pack_end(GTK_BOX(vbox), drawing, TRUE, TRUE, 0);
	GdkGLConfig *glconfig = gdk_gl_config_new_by_mode(
			GDK_GL_MODE_RGBA |
			GDK_GL_MODE_DEPTH |
			GDK_GL_MODE_DOUBLE);
	if (!glconfig)
		g_assert_not_reached();
	if (!gtk_widget_set_gl_capability(drawing, glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE))
		g_assert_not_reached();

	/* Set up tab area */
	GtkWidget *tab_area = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tab_area), GTK_POS_BOTTOM);
	gtk_paned_pack2(GTK_PANED(paned), tab_area, FALSE, FALSE);
	//GtkWidget *label = gtk_label_new("Hello");
	//GtkWidget *contents = gtk_label_new("World");
	//gtk_notebook_append_page(GTK_NOTEBOOK(tab_area), contents, label);

	/* Load plugins */
	opengl_init (GTK_DRAWING_AREA(drawing), GTK_NOTEBOOK(tab_area));
	ridge_init  (GTK_DRAWING_AREA(drawing), GTK_NOTEBOOK(tab_area));
	radar_init  (GTK_DRAWING_AREA(drawing), GTK_NOTEBOOK(tab_area));
	example_init(GTK_DRAWING_AREA(drawing), GTK_NOTEBOOK(tab_area));

	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}

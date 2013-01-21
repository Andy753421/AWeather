#include <gtk/gtk.h>

#if ! GTK_CHECK_VERSION(3,0,0)
static inline GtkWidget *gtk_box_new(GtkOrientation orientation, gint spacing)
{
	return orientation == GTK_ORIENTATION_HORIZONTAL
		? gtk_hbox_new(0, spacing)
		: gtk_vbox_new(0, spacing);
}
static inline GtkWidget *gtk_scale_new_with_range(GtkOrientation orientation,
		gdouble min, gdouble max, gdouble step)
{
	return orientation == GTK_ORIENTATION_HORIZONTAL
		? gtk_hscale_new_with_range(min, max, step)
		: gtk_vscale_new_with_range(min, max, step);
}
#endif

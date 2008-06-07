#ifndef CUBE_H
#define CUBE_H

gboolean expose(GtkWidget *da, GdkEventExpose *event, gpointer user_data);
gboolean configure(GtkWidget *da, GdkEventConfigure *event, gpointer user_data);
gboolean rotate(gpointer user_data);

#endif

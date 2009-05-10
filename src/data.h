#ifndef __DATA_H__
#define __DATA_H__

typedef void (*AWeatherCacheDoneCallback)(const gchar *file, gpointer user_data);

void cache_file(char *base, char *path, AWeatherCacheDoneCallback callback, gpointer user_data);

#endif

#ifndef SITE_H
#define SITE_H

enum {
	LOCATION_CITY,
	LOCATION_STATE
};

typedef struct {
	int type;
	char *code;
	char *label;
	struct {
		gint north;
		gint ne;
		gint east;
		gint se;
		gint south;
		gint sw;
		gint west;
		gint nw;
	} neighbors;
} city_t;

extern city_t cities[];


#endif

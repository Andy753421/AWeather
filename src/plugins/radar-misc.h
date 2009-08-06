#define RSL_FOREACH_VOL(radar, volume, count, index) \
	guint count = 0; \
	for (guint index = 0; index < radar->h.nvolumes; index++) { \
		Volume *volume = radar->v[index]; \
		if (volume == NULL) continue; \
		count++;

#define RSL_FOREACH_SWEEP(volume, sweep, count, index) \
	guint count = 0; \
	for (guint index = 0; index < volume->h.nsweeps; index++) { \
		Sweep *sweep = volume->sweep[index]; \
		if (sweep == NULL || sweep->h.elev == 0) continue; \
		count++;

#define RSL_FOREACH_RAY(sweep, ray, count, index) \
	guint count = 0; \
	for (guint index = 0; index < sweep->h.nrays; index++) { \
		Ray *ray = sweep->ray[index]; \
		if (ray == NULL) continue; \
		count++;

#define RSL_FOREACH_BIN(ray, bin, count, index) \
	guint count = 0; \
	for (guint index = 0; index < ray->h.nbins; index++) { \
		Range bin = ray->range[index]; \
		count++;

#define RSL_FOREACH_END }

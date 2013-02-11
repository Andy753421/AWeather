/*
 * Copyright (C) 2010-2011 Andy Spencer <andy753421@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <time.h>
#include <grits.h>
#include <stdio.h>
#include <string.h>

#include "alert.h"
#include "alert-info.h"

#include "../compat.h"

#define MSG_INDEX "http://alerts.weather.gov/cap/us.php?x=0"
#define CONFIG_HEIGHT 3

/* Format for single cap alert:
 *   "http://alerts.weather.gov/cap/wwacapget.php?x="
 *   "AK20111012000500CoastalFloodWarning20111012151500"
 *   "AKAFGCFWAFG.6258a84eb1e8c34cd888057248224d10" */

/************
 * AlertMsg *
 ************/
/* AlertMsg data types */
typedef struct {
	char sep0;
	char class [1 ]; char sep1;
	char action[3 ]; char sep2;
	char office[4 ]; char sep3;
	char phenom[2 ]; char sep4;
	char signif[1 ]; char sep5;
	char event [4 ]; char sep6;
	char begin [12]; char sep7;
	char end   [12]; char sep8;
} AlertVtec;

typedef struct {
	/* Basic info (from alert index) */
	char *title;   // Winter Weather Advisory issued December 19 at 8:51PM
	char *link;    // http://www.weather.gov/alerts-beta/wwacapget.php?x=AK20101219205100AFGWinterWeatherAdvisoryAFG20101220030000AK
	char *summary; // ...WINTER WEATHER ADVISORY REMAINS IN EFFECT UNTIL 6
	struct {
		time_t     effective; // 2010-12-19T20:51:00-09:00
		time_t     expires;   // 2010-12-20T03:00:00-09:00
		char      *status;    // Actual
		char      *urgency;   // Expected
		char      *severity;  // Minor
		char      *certainty; // Likely
		char      *area_desc; // Northeastern Brooks Range; Northwestern Brooks Range
		char      *fips6;     // 006015 006023 006045 006105
		AlertVtec *vtec;      // /X.CON.PAFG.WW.Y.0064.000000T0000Z-101220T0300Z/
	} cap;

	/* Advanced info (from CAP file) */
	char *description;
	char *instruction;
	char *polygon;

	/* Internal state */
	AlertInfo *info;         // Link to info structure for this alert
	GritsPoly *county_based; // Polygon for county-based warning
	GritsPoly *storm_based;  // Polygon for storm-based warning
} AlertMsg;

/* AlertMsg parsing */
typedef struct {
	time_t    updated;
	AlertMsg *msg;
	GList    *msgs;
	gchar    *text;
	gchar    *value_name;
} ParseData;
time_t msg_parse_time(gchar *iso8601)
{
	GTimeVal tv = {};
	g_time_val_from_iso8601(iso8601, &tv);
	return tv.tv_sec;
}
AlertVtec *msg_parse_vtec(char *buf)
{
	AlertVtec *vtec = g_new0(AlertVtec, 1);
	strncpy((char*)vtec, buf, sizeof(AlertVtec));
	vtec->sep0 = vtec->sep1 = vtec->sep2 = '\0';
	vtec->sep3 = vtec->sep4 = vtec->sep5 = '\0';
	vtec->sep6 = vtec->sep7 = vtec->sep8 = '\0';
	return vtec;
}
void msg_parse_text(GMarkupParseContext *context, const gchar *text,
		gsize len, gpointer user_data, GError **error)
{
	//g_debug("text %s", text);
	ParseData *data = user_data;
	if (data->text)
		g_free(data->text);
	data->text = g_strndup(text, len);
}
void msg_parse_index_start(GMarkupParseContext *context, const gchar *name,
		const gchar **keys, const gchar **vals,
		gpointer user_data, GError **error)
{
	//g_debug("start %s", name);
	ParseData *data = user_data;
	if (g_str_equal(name, "entry"))
		data->msg  = g_new0(AlertMsg, 1);
}
void msg_parse_index_end(GMarkupParseContext *context, const gchar *name,
		gpointer user_data, GError **error)
{
	//g_debug("end %s", name);
	ParseData *data = user_data;
	AlertMsg  *msg  = data->msg;
	char      *text = data->text;

	if (g_str_equal(name, "updated") && text && !data->updated)
		data->updated = msg_parse_time(text);

	if (g_str_equal(name, "entry"))
		data->msgs = g_list_prepend(data->msgs, data->msg);

	if (!text || !msg) return;
	else if (g_str_equal(name, "title"))         msg->title         = g_strdup(text);
	else if (g_str_equal(name, "id"))            msg->link          = g_strdup(text); // hack
	else if (g_str_equal(name, "summary"))       msg->summary       = g_strdup(text);
	else if (g_str_equal(name, "cap:effective")) msg->cap.effective = msg_parse_time(text);
	else if (g_str_equal(name, "cap:expires"))   msg->cap.expires   = msg_parse_time(text);
	else if (g_str_equal(name, "cap:status"))    msg->cap.status    = g_strdup(text);
	else if (g_str_equal(name, "cap:urgency"))   msg->cap.urgency   = g_strdup(text);
	else if (g_str_equal(name, "cap:severity"))  msg->cap.severity  = g_strdup(text);
	else if (g_str_equal(name, "cap:certainty")) msg->cap.certainty = g_strdup(text);
	else if (g_str_equal(name, "cap:areaDesc"))  msg->cap.area_desc = g_strdup(text);

	if (g_str_equal(name, "title"))
		msg->info = alert_info_find(msg->title);

	if (g_str_equal(name, "valueName")) {
		if (data->value_name)
			g_free(data->value_name);
		data->value_name = g_strdup(text);
	}

	if (g_str_equal(name, "value") && data->value_name) {
		if (g_str_equal(data->value_name, "FIPS6")) msg->cap.fips6 = g_strdup(text);
		if (g_str_equal(data->value_name, "VTEC"))  msg->cap.vtec  = msg_parse_vtec(text);
	}
}
void msg_parse_cap_end(GMarkupParseContext *context, const gchar *name,
		gpointer user_data, GError **error)
{
	ParseData *data = user_data;
	AlertMsg  *msg  = data->msg;
	char      *text = data->text;
	if (!text || !msg) return;
	if      (g_str_equal(name, "description")) msg->description = g_strdup(text);
	else if (g_str_equal(name, "instruction")) msg->instruction = g_strdup(text);
	else if (g_str_equal(name, "polygon"))     msg->polygon     = g_strdup(text);
}

/* AlertMsg methods */
GList *msg_parse_index(gchar *text, gsize len, time_t *updated)
{
	g_debug("GritsPluginAlert: msg_parse");
	GMarkupParser parser = {
		.start_element = msg_parse_index_start,
		.end_element   = msg_parse_index_end,
		.text          = msg_parse_text,
	};
	ParseData data = {};
	GMarkupParseContext *context =
		g_markup_parse_context_new(&parser, 0, &data, NULL);
	g_markup_parse_context_parse(context, text, len, NULL);
	g_markup_parse_context_free(context);
	if (data.text)
		g_free(data.text);
	if (data.value_name)
		g_free(data.value_name);
	*updated = data.updated;
	return data.msgs;
}

void msg_parse_cap(AlertMsg *msg, gchar *text, gsize len)
{
	g_debug("GritsPluginAlert: msg_parse_cap");
	GMarkupParser parser = {
		.end_element   = msg_parse_cap_end,
		.text          = msg_parse_text,
	};
	ParseData data = { .msg = msg };
	GMarkupParseContext *context =
		g_markup_parse_context_new(&parser, 0, &data, NULL);
	g_markup_parse_context_parse(context, text, len, NULL);
	g_markup_parse_context_free(context);
	if (data.text)
		g_free(data.text);

	/* Add spaces to description... */
	static GRegex *regex = NULL;
	if (regex == NULL)
		regex = g_regex_new("\\.\\n", 0, G_REGEX_MATCH_NEWLINE_ANY, NULL);
	if (msg->description && regex) {
		char *old = msg->description;
		msg->description = g_regex_replace_literal(
				regex, old, -1, 0, ".\n\n", 0, NULL);
		g_free(old);
	}
}

void msg_free(AlertMsg *msg)
{
	g_free(msg->title);
	g_free(msg->link);
	g_free(msg->summary);
	g_free(msg->cap.status);
	g_free(msg->cap.urgency);
	g_free(msg->cap.severity);
	g_free(msg->cap.certainty);
	g_free(msg->cap.area_desc);
	g_free(msg->cap.fips6);
	g_free(msg->cap.vtec);
	g_free(msg->description);
	g_free(msg->instruction);
	g_free(msg->polygon);
	g_free(msg);
}

void msg_print(GList *msgs)
{
	g_message("msg_print");
	for (GList *cur = msgs; cur; cur = cur->next) {
		AlertMsg *msg = cur->data;
		g_message("alert:");
		g_message("	title         = %s",  msg->title        );
		g_message("	link          = %s",  msg->link         );
		g_message("	summary       = %s",  msg->summary      );
		g_message("	cat.effective = %lu", msg->cap.effective);
		g_message("	cat.expires   = %lu", msg->cap.expires  );
		g_message("	cat.status    = %s",  msg->cap.status   );
		g_message("	cat.urgency   = %s",  msg->cap.urgency  );
		g_message("	cat.severity  = %s",  msg->cap.severity );
		g_message("	cat.certainty = %s",  msg->cap.certainty);
		g_message("	cat.area_desc = %s",  msg->cap.area_desc);
		g_message("	cat.fips6     = %s",  msg->cap.fips6    );
		g_message("	cat.vtec      = %p",  msg->cap.vtec     );
	}
}

gchar *msg_find_nearest(GritsHttp *http, time_t when, gboolean offline)
{
	GList *files = grits_http_available(http,
			"^[0-9]*.xml$", "index", NULL, NULL);

	time_t  this_time    = 0;
	time_t  nearest_time = offline ? 0 : time(NULL);
	char   *nearest_file = NULL;

	for (GList *cur = files; cur; cur = cur->next) {
		gchar *file = cur->data;
		sscanf(file, "%ld", &this_time);
		if (ABS(when - this_time) <
		    ABS(when - nearest_time)) {
			nearest_file = file;
			nearest_time = this_time;
		}
	}

	if (nearest_file)
		return g_strconcat("index/", nearest_file, NULL);
	else if (!offline)
		return g_strdup_printf("index/%ld.xml", time(NULL));
	else
		return NULL;
}

GList *msg_load_index(GritsHttp *http, time_t when, time_t *updated, gboolean offline)
{
	/* Fetch current alerts */
	gchar *tmp = msg_find_nearest(http, when, offline);
	if (!tmp)
		return NULL;
	gchar *file = grits_http_fetch(http, MSG_INDEX, tmp, GRITS_ONCE, NULL, NULL);
	g_free(tmp);
	if (!file)
		return NULL;

	/* Load file */
	gchar *text; gsize len;
	g_file_get_contents(file, &text, &len, NULL);
	GList *msgs = msg_parse_index(text, len, updated);
	//msg_print(msgs);
	g_free(file);
	g_free(text);

	/* Delete unrecognized messages */
	GList *dead = NULL;
	for (GList *cur = msgs; cur; cur = cur->next)
		if (!((AlertMsg*)cur->data)->info)
			dead = g_list_prepend(dead, cur->data);
	for (GList *cur = dead; cur; cur = cur->next) {
		AlertMsg *msg = cur->data;
		g_warning("GritsPluginAlert: unknown msg type - %s", msg->title);
		msgs = g_list_remove(msgs, msg);
		msg_free(msg);
	}
	g_list_free(dead);

	return msgs;
}

gboolean msg_load_cap(GritsHttp *http, AlertMsg *msg)
{
	if (msg->description || msg->instruction || msg->polygon)
		return TRUE;
	g_debug("GritsPlguinAlert: update_cap");

	/* Download */
	gchar *id = strrchr(msg->link, '=');
	if (!id) return FALSE; id++;
	gchar *dir  = g_strdelimit(g_strdup(msg->info->abbr), " ", '_');
	gchar *tmp  = g_strdup_printf("%s/%s.xml", dir, id);
	gchar *file = grits_http_fetch(http, msg->link, tmp, GRITS_ONCE, NULL, NULL);
	g_free(tmp);
	g_free(dir);
	if (!file)
		return FALSE;

	/* Parse */
	gchar *text; gsize len;
	g_file_get_contents(file, &text, &len, NULL);
	msg_parse_cap(msg, text, len);
	g_free(file);
	g_free(text);
	return TRUE;
}


/********
 * FIPS *
 ********/
int fips_compare(int a, int b)
{
	return (a <  b) ? -1 :
	       (a == b) ?  0 : 1;
}

GritsPoly *fips_combine(GList *polys)
{
	/* Create points list */
	GPtrArray *array = g_ptr_array_new();
	for (GList *cur = polys; cur; cur = cur->next) {
		GritsPoly *poly       = cur->data;
		gdouble (**points)[3] = poly->points;
		for (int i = 0; points[i]; i++)
			g_ptr_array_add(array, points[i]);
	}
	g_ptr_array_add(array, NULL);
	gdouble (**points)[3] = (gpointer)g_ptr_array_free(array, FALSE);

	/* Calculate center */
	GritsBounds bounds = {-90, 90, -180, 180};
	for (GList *cur = polys; cur; cur = cur->next) {
		GritsObject *poly = cur->data;
		gdouble lat = poly->center.lat;
		gdouble lon = poly->center.lon;
		if (lat > bounds.n) bounds.n = lat;
		if (lat < bounds.s) bounds.s = lat;
		if (lon > bounds.e) bounds.e = lon;
		if (lon < bounds.w) bounds.w = lon;
	}
	GritsPoint center = {
		.lat = (bounds.n + bounds.s)/2,
		.lon = lon_avg(bounds.e, bounds.w),
	};

	/* Create polygon */
	GritsPoly *poly = grits_poly_new(points);
	GRITS_OBJECT(poly)->skip  |= GRITS_SKIP_CENTER;
	GRITS_OBJECT(poly)->skip  |= GRITS_SKIP_STATE;
	GRITS_OBJECT(poly)->center = center;
	g_object_weak_ref(G_OBJECT(poly), (GWeakNotify)g_free, points);
	return poly;
}

gboolean fips_group_state(gpointer key, gpointer value, gpointer data)
{
	GList  *counties = value;
	GList **states   = data;
	GritsPoly *poly  = fips_combine(counties);
	GRITS_OBJECT(poly)->lod = EARTH_R/10;
	*states = g_list_prepend(*states, poly);
	g_list_free(counties);
	return FALSE;
}

void fips_parse(gchar *text, GTree **_counties, GList **_states)
{
	g_debug("GritsPluginAlert: fips_parse");
	GTree *counties = g_tree_new((GCompareFunc)fips_compare);
	GTree *states   = g_tree_new_full((GCompareDataFunc)g_strcmp0,
			NULL, g_free, NULL);
	gchar **lines = g_strsplit(text, "\n", -1);
	for (gint li = 0; lines[li]; li++) {
		/* Split line */
		gchar **sparts = g_strsplit(lines[li], "\t", 4);
		int     nparts = g_strv_length(sparts);
		if (nparts < 4) {
			g_strfreev(sparts);
			continue;
		}

		/* Create GritsPoly */
		GritsPoly *poly = grits_poly_parse(sparts[3], "\t", " ", ",");

		/* Insert polys into the tree */
		glong id = g_ascii_strtoll(sparts[0], NULL, 10);
		g_tree_insert(counties, (gpointer)id, poly);

		/* Insert into states list */
		GList *list = g_tree_lookup(states, sparts[2]);
		list = g_list_prepend(list, poly);
		g_tree_replace(states, g_strdup(sparts[2]), list);

		g_strfreev(sparts);
	}
	g_strfreev(lines);

	/* Group state counties */
	*_counties = counties;
	*_states   = NULL;
	g_tree_foreach(states, (GTraverseFunc)fips_group_state, _states);
	g_tree_destroy(states);
}

/********************
 * GritsPluginAlert *
 ********************/
static GtkWidget *_make_msg_details(AlertMsg *msg)
{

	GtkWidget *title     = gtk_label_new("");
	gchar     *title_str = g_markup_printf_escaped("<big><b>%s</b></big>",
			msg->title ?: "No title provided");
	gtk_label_set_use_markup(GTK_LABEL(title), TRUE);
	gtk_label_set_markup(GTK_LABEL(title), title_str);
	gtk_label_set_line_wrap(GTK_LABEL(title), TRUE);
	gtk_misc_set_alignment(GTK_MISC(title), 0, 0);
	gtk_widget_set_size_request(GTK_WIDGET(title), 500, -1);
	g_free(title_str);

	GtkWidget     *alert      = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget     *alert_view = gtk_text_view_new();
	GtkTextBuffer *alert_buf  = gtk_text_buffer_new(NULL);
	gchar         *alert_str  = g_markup_printf_escaped("%s\n\n%s",
			msg->description ?: "No description provided",
			msg->instruction ?: "No instructions provided");
	PangoFontDescription *alert_font = pango_font_description_from_string(
			"monospace");
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(alert),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(alert),
			GTK_SHADOW_IN);
	gtk_text_buffer_set_text(alert_buf, alert_str, -1);
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(alert_view), alert_buf);
	gtk_widget_modify_font(GTK_WIDGET(alert_view), alert_font);
	gtk_container_add(GTK_CONTAINER(alert), alert_view);
	g_free(alert_str);

	GtkWidget *align = gtk_alignment_new(0, 0, 1, 1);
	GtkWidget *box   = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align), 10, 10, 10, 10);
	gtk_container_add(GTK_CONTAINER(align), box);
	gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), alert, TRUE,  TRUE,  0);

	return align;
}

static GtkWidget *_find_details(GtkNotebook *notebook, AlertMsg *msg)
{
	int pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
	for (int i = 0; i < pages; i++) {
		GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), i);
		if (msg == g_object_get_data(G_OBJECT(page), "msg"))
			return page;
	}
	return NULL;
}

static gboolean _show_details(GritsPoly *county, GdkEvent *_, GritsPluginAlert *alert)
{
	/* Add details for this messages */
	AlertMsg *msg = g_object_get_data(G_OBJECT(county), "msg");

	// TODO: move this to a thread since it blocks on net access
	if (!msg_load_cap(alert->http, msg))
		return FALSE;

	GtkWidget *dialog   = alert->details;
	GtkWidget *content  = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	GList     *list     = gtk_container_get_children(GTK_CONTAINER(content));
	GtkWidget *notebook = list->data;
	GtkWidget *details  = _find_details(GTK_NOTEBOOK(notebook), msg);
	g_list_free(list);

	if (details == NULL) {
		details          = _make_msg_details(msg);
		GtkWidget *label = gtk_label_new(msg->info->abbr);
		g_object_set_data(G_OBJECT(details), "msg", msg);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), details, label);
	}

	gtk_widget_show_all(dialog);
	gint num = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), details);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), num);

	return FALSE;
}

/* Update counties */
static gboolean _alert_leave(GritsPoly *county, GdkEvent *_, GritsPluginAlert *alert)
{
	g_debug("_alert_leave");
	if (county->width == 3) {
		county->color[3]  = 0;
	} else {
		county->border[3] = 0.25;
		county->width     = 1;
	}
	grits_object_queue_draw(GRITS_OBJECT(county));
	return FALSE;
}

static gboolean _alert_enter(GritsPoly *county, GdkEvent *_, GritsPluginAlert *alert)
{
	g_debug("_alert_enter");
	if (county->width == 3) {
		county->color[3]  = 0.25;
	} else {
		county->border[3] = 1.0;
		county->width     = 2;
	}
	grits_object_queue_draw(GRITS_OBJECT(county));
	return FALSE;
}

/* Update polygons */
static void _load_common(GritsPluginAlert *alert, AlertMsg *msg, GritsPoly *poly,
		float color, float border, int width, gchar *hidden)
{
	g_object_set_data(G_OBJECT(poly), "msg", msg);
	poly->color[0]  = poly->border[0] = (float)msg->info->color[0] / 256;
	poly->color[1]  = poly->border[1] = (float)msg->info->color[1] / 256;
	poly->color[2]  = poly->border[2] = (float)msg->info->color[2] / 256;
	poly->color[3]  = color;
	poly->border[3] = border;
	poly->width     = width;
	GRITS_OBJECT(poly)->lod    = 0;
	GRITS_OBJECT(poly)->hidden = msg->info->hidden ||
		grits_prefs_get_boolean(alert->prefs, hidden, NULL);
	g_signal_connect(poly, "enter",   G_CALLBACK(_alert_enter),  alert);
	g_signal_connect(poly, "leave",   G_CALLBACK(_alert_leave),  alert);
	g_signal_connect(poly, "clicked", G_CALLBACK(_show_details), alert);
}

static GritsPoly *_load_storm_based(GritsPluginAlert *alert, AlertMsg *msg)
{
	if (!msg->info->ispoly)
		return NULL;

	if (!msg_load_cap(alert->http, msg))
		return NULL;

	if (!msg->polygon)
		return NULL;

	GritsPoly *poly = grits_poly_parse(msg->polygon, "\t", " ", ",");
	_load_common(alert, msg, poly, 0, 1, 3, "alert/hide_storm_based");
	grits_viewer_add(alert->viewer, GRITS_OBJECT(poly), GRITS_LEVEL_WORLD+4, FALSE);

	return poly;
}

static GritsPoly *_load_county_based(GritsPluginAlert *alert, AlertMsg *msg)
{
	/* Locate counties in the path of the storm */
	gchar **fipses  = g_strsplit(msg->cap.fips6, " ", -1);
	GList *counties = NULL;
	for (int i = 0; fipses[i]; i++) {
		glong fips = g_ascii_strtoll(fipses[i], NULL, 10);
		GritsPoly *county = g_tree_lookup(alert->counties, (gpointer)fips);
		if (!county)
			continue;
		counties = g_list_prepend(counties, county);
	}
	g_strfreev(fipses);

	/* No county based warning.. */
	if (!counties)
		return NULL;

	/* Create new county based warning */
	GritsPoly *poly = fips_combine(counties);
	_load_common(alert, msg, poly, 0.25, 0.25, 0, "alert/hide_county_based");
	grits_viewer_add(alert->viewer, GRITS_OBJECT(poly), GRITS_LEVEL_WORLD+1, FALSE);

	g_list_free(counties);
	return poly;
}

/* Update buttons */
static gboolean _show_hide(GtkToggleButton *button, GritsPluginAlert *alert)
{
	g_debug("GritsPluginAlert: _show_hide - alert=%p, config=%p", alert, alert->config);

	/* Check if we've clicked a alert type button */
	AlertInfo *info = g_object_get_data(G_OBJECT(button), "info");
	if (info)
		info->hidden = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	/* Update county/storm based hiding */
	GtkWidget *ctoggle = g_object_get_data(G_OBJECT(alert->config), "county_based");
	GtkWidget *stoggle = g_object_get_data(G_OBJECT(alert->config), "storm_based");

	gboolean   cshow   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ctoggle));
	gboolean   sshow   = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(stoggle));

	grits_prefs_set_boolean(alert->prefs, "alert/hide_county_based", !cshow);
	grits_prefs_set_boolean(alert->prefs, "alert/hide_storm_based",  !sshow);

	/* Show/hide each message */
	for (GList *cur = alert->msgs; cur; cur = cur->next) {
		AlertMsg *msg = cur->data;
		gboolean hide = msg->info->hidden;
		if (msg->county_based)
			GRITS_OBJECT(msg->county_based)->hidden = !cshow || hide;
		if (msg->storm_based)
			GRITS_OBJECT(msg->storm_based)->hidden  = !sshow || hide;
	}

	grits_viewer_queue_draw(alert->viewer);
	return TRUE;
}


static GtkWidget *_button_new(AlertInfo *info)
{
	g_debug("GritsPluginAlert: _button_new - %s", info->title);
	GdkColor black = {0, 0, 0, 0};
	GdkColor color = {0, info->color[0]<<8, info->color[1]<<8, info->color[2]<<8};

	gchar text[64];
	g_snprintf(text, sizeof(text), "<b>%.10s</b>", info->abbr);

	GtkWidget *button = gtk_toggle_button_new();
	GtkWidget *align  = gtk_alignment_new(0.5, 0.5, 1, 1);
	GtkWidget *cbox   = gtk_event_box_new();
	GtkWidget *label  = gtk_label_new(text);
	for (int state = 0; state < GTK_STATE_INSENSITIVE; state++) {
		gtk_widget_modify_fg(label, state, &black);
		gtk_widget_modify_bg(cbox,  state, &color);
	} /* Yuck.. */
	g_object_set_data(G_OBJECT(button), "info", info);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align), 2, 2, 4, 4);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), !info->hidden);
	gtk_widget_set_tooltip_text(GTK_WIDGET(button), info->title);
	gtk_container_add(GTK_CONTAINER(cbox), label);
	gtk_container_add(GTK_CONTAINER(align), cbox);
	gtk_container_add(GTK_CONTAINER(button), align);
	return button;
}

static gboolean _update_buttons(GritsPluginAlert *alert)
{
	g_debug("GritsPluginAlert: _update_buttons");
	GtkWidget *alerts  = g_object_get_data(G_OBJECT(alert->config), "alerts");
	GtkWidget *updated = g_object_get_data(G_OBJECT(alert->config), "updated");

	/* Determine which buttons to show */
	for (int i = 0; alert_info[i].title; i++)
		alert_info[i].current = FALSE;
	for (GList *cur = alert->msgs; cur; cur = cur->next) {
		AlertMsg *msg = cur->data;
		msg->info->current = TRUE;
	}

	/* Delete old buttons */
	GList *frames = gtk_container_get_children(GTK_CONTAINER(alerts));
	for (GList *frame = frames; frame; frame = frame->next) {
		GtkWidget *table = gtk_bin_get_child(GTK_BIN(frame->data));
		GList *btns = gtk_container_get_children(GTK_CONTAINER(table));
		g_list_foreach(btns, (GFunc)gtk_widget_destroy, NULL);
		g_list_free(btns);
	}
	g_list_free(frames);

	/* Add new buttons */
	for (int i = 0; alert_info[i].title; i++) {
		if (!alert_info[i].current)
			continue;

		GtkWidget *table = g_object_get_data(G_OBJECT(alerts),
				alert_info[i].category);
		GList *kids = gtk_container_get_children(GTK_CONTAINER(table));
		gint  nkids = g_list_length(kids);
		gint x = nkids / CONFIG_HEIGHT;
		gint y = nkids % CONFIG_HEIGHT;
		g_list_free(kids);

		GtkWidget *button = _button_new(&alert_info[i]);
		gtk_table_attach(GTK_TABLE(table), button, x, x+1, y, y+1,
				GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
		g_signal_connect(button, "clicked", G_CALLBACK(_show_hide), alert);
	}

	/* Set time widget */
	struct tm *tm = gmtime(&alert->updated);
	gchar *date_str = g_strdup_printf(" <b><i>%04d-%02d-%02d %02d:%02d</i></b>",
			tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
			tm->tm_hour,      tm->tm_min);
	gtk_label_set_markup(GTK_LABEL(updated), date_str);
	g_free(date_str);

	gtk_widget_show_all(GTK_WIDGET(alert->config));
	alert->update_source = 0;
	return FALSE;
}

static gint _sort_warnings(gconstpointer _a, gconstpointer _b)
{
	const AlertMsg *a=_a, *b=_b;
	return (a->info->prior <  b->info->prior) ? -1 :
	       (a->info->prior == b->info->prior) ?  0 : 1;
}

static void _update_warnings(GritsPluginAlert *alert, GList *old)
{
	g_debug("GritsPluginAlert: _update_warnings");

	/* Sort by priority:
	 *   reversed here since they're added
	 *   to the viewer in reverse order */
	alert->msgs = g_list_sort(alert->msgs, _sort_warnings);

	/* Remove old messages */
	for (GList *cur = old; cur; cur = cur->next) {
		AlertMsg *msg = cur->data;
		grits_object_destroy_pointer(&msg->county_based);
		grits_object_destroy_pointer(&msg->storm_based);
	}

	/* Add new messages */
	/* Load counties first since it does not require network access */
	for (GList *cur = alert->msgs; cur; cur = cur->next) {
		AlertMsg *msg = cur->data;
		msg->county_based = _load_county_based(alert, msg);
	}
	grits_viewer_queue_draw(alert->viewer);
	for (GList *cur = alert->msgs; cur; cur = cur->next) {
		AlertMsg *msg = cur->data;
		msg->storm_based  = _load_storm_based(alert, msg);
		if (alert->aborted)
			return;
	}
	grits_viewer_queue_draw(alert->viewer);

	g_debug("GritsPluginAlert: _load_warnings - end");
}

/* Callbacks */
static void _update(gpointer _, gpointer _alert)
{
	GritsPluginAlert *alert = _alert;
	if (alert->aborted)
		return;
	GList *old = alert->msgs;
	g_debug("GritsPluginAlert: _update");

	time_t   when    = grits_viewer_get_time(alert->viewer);
	gboolean offline = grits_viewer_get_offline(alert->viewer);
	if (!(alert->msgs = msg_load_index(alert->http, when, &alert->updated, offline)))
		return;

	if (!alert->update_source)
		alert->update_source = g_idle_add((GSourceFunc)_update_buttons, alert);
	_update_warnings(alert, old);

	g_list_foreach(old, (GFunc)msg_free, NULL);
	g_list_free(old);

	g_debug("GritsPluginAlert: _update - end");
}

static void _on_update(GritsPluginAlert *alert)
{
	g_thread_pool_push(alert->threads, NULL+1, NULL);
}

/* Init helpers */
static GtkWidget *_make_config(GritsPluginAlert *alert)
{
	GtkWidget *config = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	/* Setup tools area */
	GtkWidget *tools   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	GtkWidget *updated = gtk_label_new(" Loading...");
	GtkWidget *storm_based  = gtk_toggle_button_new_with_label("Storm based");
	GtkWidget *county_based = gtk_toggle_button_new_with_label("County based");
	gtk_label_set_use_markup(GTK_LABEL(updated), TRUE);
	gtk_box_pack_start(GTK_BOX(tools), updated,      FALSE, FALSE, 0);
	gtk_box_pack_end  (GTK_BOX(tools), storm_based,  FALSE, FALSE, 0);
	gtk_box_pack_end  (GTK_BOX(tools), county_based, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(config), tools, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(config), "updated",      updated);
	g_object_set_data(G_OBJECT(config), "storm_based",  storm_based);
	g_object_set_data(G_OBJECT(config), "county_based", county_based);
	g_signal_connect(storm_based,  "toggled", G_CALLBACK(_show_hide), alert);
	g_signal_connect(county_based, "toggled", G_CALLBACK(_show_hide), alert);

	/* Setup alerts */
	gchar *labels[] = {"<b>Warnings</b>", "<b>Watches</b>",
	                   "<b>Advisories</b>", "<b>Other</b>"};
	gchar *keys[]   = {"warning",  "watch",   "advisory",   "other"};
	GtkWidget *alerts = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	for (int i = 0; i < G_N_ELEMENTS(labels); i++) {
		GtkWidget *frame = gtk_frame_new(labels[i]);
		GtkWidget *table = gtk_table_new(1, 1, TRUE);
		GtkWidget *label = gtk_frame_get_label_widget(GTK_FRAME(frame));
		gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
		gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
		gtk_container_add(GTK_CONTAINER(frame), table);
		gtk_box_pack_start(GTK_BOX(alerts), frame, TRUE, TRUE, 0);
		g_object_set_data(G_OBJECT(alerts), keys[i], table);
	}
	gtk_box_pack_start(GTK_BOX(config), alerts, TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(config), "alerts",  alerts);

	return config;
}

static gboolean _clear_details(GtkWidget *dialog)
{
	GtkWidget *content  = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	GList     *list     = gtk_container_get_children(GTK_CONTAINER(content));
	GtkWidget *notebook = list->data;
	g_list_free(list);
	gtk_widget_hide(dialog);
	while (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)))
		gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), 0);
	return TRUE;
}

static gboolean _set_details_uri(GtkWidget *notebook, gpointer _,
		guint num, GtkWidget *button)
{
	g_debug("_set_details_uri");
	GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), num);
	AlertMsg  *msg  = g_object_get_data(G_OBJECT(page), "msg");
	gtk_link_button_set_uri(GTK_LINK_BUTTON(button), msg->link);
	return FALSE;
}

static GtkWidget *_make_details(GritsViewer *viewer)
{
	GtkWidget *dialog   = gtk_dialog_new();
	GtkWidget *action   = gtk_dialog_get_action_area (GTK_DIALOG(dialog));
	GtkWidget *content  = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	GtkWidget *notebook = gtk_notebook_new();
	GtkWidget *win      = gtk_widget_get_toplevel(GTK_WIDGET(viewer));
	GtkWidget *link     = gtk_link_button_new_with_label("", "Full Text");

	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));
	gtk_window_set_title(GTK_WINDOW(dialog), "Alert Details - AWeather");
	gtk_window_set_default_size(GTK_WINDOW(dialog), 625, 500);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
	gtk_container_add(GTK_CONTAINER(content), notebook);
	gtk_box_pack_end(GTK_BOX(action), link, 0, 0, 0);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

	g_signal_connect(dialog,   "response",     G_CALLBACK(_clear_details),   NULL);
	g_signal_connect(dialog,   "delete-event", G_CALLBACK(_clear_details),   NULL);
	g_signal_connect(notebook, "switch-page",  G_CALLBACK(_set_details_uri), link);

	return dialog;
}

/* Methods */
GritsPluginAlert *grits_plugin_alert_new(GritsViewer *viewer, GritsPrefs *prefs)
{
	g_debug("GritsPluginAlert: new");
	GritsPluginAlert *alert = g_object_new(GRITS_TYPE_PLUGIN_ALERT, NULL);
	alert->details = _make_details(viewer);
	alert->viewer  = g_object_ref(viewer);
	alert->prefs   = g_object_ref(prefs);

	alert->refresh_id      = g_signal_connect_swapped(alert->viewer, "refresh",
			G_CALLBACK(_on_update), alert);
	alert->time_changed_id = g_signal_connect_swapped(alert->viewer, "time_changed",
			G_CALLBACK(_on_update), alert);

	for (GList *cur = alert->states; cur; cur = cur->next)
		grits_viewer_add(viewer, cur->data, GRITS_LEVEL_WORLD+1, FALSE);

	gboolean   chide   = grits_prefs_get_boolean(alert->prefs, "alert/hide_county_based", NULL);
	gboolean   shide   = grits_prefs_get_boolean(alert->prefs, "alert/hide_storm_based",  NULL);
	GtkWidget *ctoggle = g_object_get_data(G_OBJECT(alert->config), "county_based");
	GtkWidget *stoggle = g_object_get_data(G_OBJECT(alert->config), "storm_based");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctoggle), !chide);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stoggle), !shide);

	_on_update(alert);
	return alert;
}

static GtkWidget *grits_plugin_alert_get_config(GritsPlugin *_alert)
{
	GritsPluginAlert *alert = GRITS_PLUGIN_ALERT(_alert);
	return alert->config;
}


/* GObject code */
static void grits_plugin_alert_plugin_init(GritsPluginInterface *iface);
G_DEFINE_TYPE_WITH_CODE(GritsPluginAlert, grits_plugin_alert, G_TYPE_OBJECT,
		G_IMPLEMENT_INTERFACE(GRITS_TYPE_PLUGIN,
			grits_plugin_alert_plugin_init));
static void grits_plugin_alert_plugin_init(GritsPluginInterface *iface)
{
	g_debug("GritsPluginAlert: plugin_init");
	/* Add methods to the interface */
	iface->get_config = grits_plugin_alert_get_config;
}
static void grits_plugin_alert_init(GritsPluginAlert *alert)
{
	g_debug("GritsPluginAlert: init");
	/* Set defaults */
	alert->threads = g_thread_pool_new(_update, alert, 1, FALSE, NULL);
	alert->config  = _make_config(alert);
	alert->http    = grits_http_new(G_DIR_SEPARATOR_S
			"alerts" G_DIR_SEPARATOR_S
			"cap"    G_DIR_SEPARATOR_S);

	/* Load counties */
	gchar *text; gsize len;
	const gchar *file = PKGDATADIR G_DIR_SEPARATOR_S "fips.txt";
	if (!g_file_get_contents(file, &text, &len, NULL))
		g_error("GritsPluginAlert: init - error loading fips polygons");
	fips_parse(text, &alert->counties, &alert->states);
	g_free(text);
}
static void grits_plugin_alert_dispose(GObject *gobject)
{
	g_debug("GritsPluginAlert: dispose");
	GritsPluginAlert *alert = GRITS_PLUGIN_ALERT(gobject);
	alert->aborted = TRUE;
	/* Drop references */
	if (alert->viewer) {
		GritsViewer *viewer = alert->viewer;
		g_signal_handler_disconnect(viewer, alert->refresh_id);
		g_signal_handler_disconnect(viewer, alert->time_changed_id);
		grits_http_abort(alert->http);
		g_thread_pool_free(alert->threads, TRUE, TRUE);
		if (alert->update_source)
			g_source_remove(alert->update_source);
		alert->viewer = NULL;
		for (GList *cur = alert->msgs; cur; cur = cur->next) {
			AlertMsg *msg = cur->data;
			grits_object_destroy_pointer(&msg->county_based);
			grits_object_destroy_pointer(&msg->storm_based);
		}
		for (GList *cur = alert->states; cur; cur = cur->next)
			grits_object_destroy_pointer(&cur->data);
		gtk_widget_destroy(alert->details);
		g_object_unref(alert->prefs);
		g_object_unref(viewer);
	}
	G_OBJECT_CLASS(grits_plugin_alert_parent_class)->dispose(gobject);
}
static gboolean _unref_county(gpointer key, gpointer val, gpointer data)
{
	g_object_unref(val);
	return FALSE;
}
static void grits_plugin_alert_finalize(GObject *gobject)
{
	g_debug("GritsPluginAlert: finalize");
	GritsPluginAlert *alert = GRITS_PLUGIN_ALERT(gobject);
	g_list_foreach(alert->msgs, (GFunc)msg_free, NULL);
	g_list_free(alert->msgs);
	g_list_free(alert->states);
	g_tree_foreach(alert->counties, (GTraverseFunc)_unref_county, NULL);
	g_tree_destroy(alert->counties);
	grits_http_free(alert->http);
	G_OBJECT_CLASS(grits_plugin_alert_parent_class)->finalize(gobject);
}
static void grits_plugin_alert_class_init(GritsPluginAlertClass *klass)
{
	g_debug("GritsPluginAlert: class_init");
	GObjectClass *gobject_class = (GObjectClass*)klass;
	gobject_class->dispose  = grits_plugin_alert_dispose;
	gobject_class->finalize = grits_plugin_alert_finalize;
}

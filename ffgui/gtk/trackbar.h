/** GUI/GTK+: trackbar
2019, Simon Zolin */

typedef struct ffui_trackbar {
	_FFUI_CTL_MEMBERS
	uint scroll_id;
} ffui_trackbar;

FF_EXTERN void _ffui_track_value_changed(GtkWidget *widget, gpointer udata);
static inline int ffui_track_create(ffui_trackbar *t, ffui_window *parent) {
	t->uid = FFUI_UID_TRACKBAR;
	t->h = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
	gtk_scale_set_draw_value(GTK_SCALE(t->h), 0);
	t->wnd = parent;
	g_signal_connect(t->h, "value-changed", (GCallback)_ffui_track_value_changed, t);
	return 0;
}

static inline void ffui_track_setrange(ffui_trackbar *t, uint max) {
	g_signal_handlers_block_by_func(t->h, (GClosure*)_ffui_track_value_changed, t);
	gtk_range_set_range(GTK_RANGE((t)->h), 0, max);
	g_signal_handlers_unblock_by_func(t->h, (GClosure*)_ffui_track_value_changed, t);
}

static inline void ffui_track_set(ffui_trackbar *t, uint val) {
	g_signal_handlers_block_by_func(t->h, (GClosure*)_ffui_track_value_changed, t);
	gtk_range_set_value(GTK_RANGE(t->h), val);
	g_signal_handlers_unblock_by_func(t->h, (GClosure*)_ffui_track_value_changed, t);
}

#define ffui_track_val(t)  gtk_range_get_value(GTK_RANGE((t)->h))

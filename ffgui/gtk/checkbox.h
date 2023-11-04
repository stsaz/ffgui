/** GUI/GTK+: checkbox
2019, Simon Zolin */

typedef struct ffui_checkbox ffui_checkbox;
struct ffui_checkbox {
	_FFUI_CTL_MEMBERS
	uint action_id;
};

FF_EXTERN void _ffui_checkbox_clicked(GtkWidget *widget, gpointer udata);
static inline int ffui_checkbox_create(ffui_checkbox *cb, ffui_window *parent) {
	cb->h = gtk_check_button_new();
	cb->wnd = parent;
	g_signal_connect(cb->h, "clicked", (GCallback)_ffui_checkbox_clicked, cb);
	return 0;
}

static inline void ffui_checkbox_settextz(ffui_checkbox *cb, const char *textz) {
	gtk_button_set_label(GTK_BUTTON(cb->h), textz);
}
static inline void ffui_checkbox_settextstr(ffui_checkbox *cb, const ffstr *text) {
	char *sz = ffsz_dupstr(text);
	gtk_button_set_label(GTK_BUTTON(cb->h), sz);
	ffmem_free(sz);
}

static inline int ffui_checkbox_checked(ffui_checkbox *cb) {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb->h));
}
static inline void ffui_checkbox_check(ffui_checkbox *cb, int val) {
	g_signal_handlers_block_by_func(cb->h, (void*)_ffui_checkbox_clicked, cb);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb->h), val);
	g_signal_handlers_unblock_by_func(cb->h, (void*)_ffui_checkbox_clicked, cb);
}

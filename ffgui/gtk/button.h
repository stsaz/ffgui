/** GUI/GTK+: button
2019, Simon Zolin */

typedef struct ffui_button ffui_button;
struct ffui_button {
	_FFUI_CTL_MEMBERS
	uint action_id;
};

FF_EXTERN void _ffui_button_clicked(GtkWidget *widget, gpointer udata);
static inline int ffui_button_create(ffui_button *b, ffui_window *parent) {
	b->h = gtk_button_new();
	b->wnd = parent;
	g_signal_connect(b->h, "clicked", (GCallback)_ffui_button_clicked, b);
	return 0;
}

static inline void ffui_button_settextz(ffui_button *b, const char *textz) {
	gtk_button_set_label(GTK_BUTTON(b->h), textz);
}
static inline void ffui_button_settextstr(ffui_button *b, const ffstr *text) {
	char *sz = ffsz_dupstr(text);
	gtk_button_set_label(GTK_BUTTON(b->h), sz);
	ffmem_free(sz);
}

static inline void ffui_button_seticon(ffui_button *b, const ffui_icon *ico) {
	GtkWidget *img = gtk_image_new();
	gtk_image_set_from_pixbuf(GTK_IMAGE(img), ico->ico);
	gtk_button_set_image(GTK_BUTTON(b->h), img);
}

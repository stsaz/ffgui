/** GUI/GTK+: image
2019, Simon Zolin */

typedef struct ffui_image {
	_FFUI_CTL_MEMBERS
} ffui_image;

static inline int ffui_image_create(ffui_image *c, ffui_window *parent) {
	c->h = gtk_image_new();
	c->wnd = parent;
	return 0;
}

static inline void ffui_image_setfile(ffui_image *c, const char *fn) {
	gtk_image_set_from_file(GTK_IMAGE(c->h), fn);
}

static inline void ffui_image_seticon(ffui_image *c, ffui_icon *ico) {
	gtk_image_set_from_pixbuf(GTK_IMAGE(c->h), ico->ico);
}

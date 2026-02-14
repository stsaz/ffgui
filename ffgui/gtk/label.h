/** GUI/GTK+: label
2019, Simon Zolin */

typedef struct ffui_label {
	_FFUI_CTL_MEMBERS
} ffui_label;

static inline int ffui_label_create(ffui_label *l, ffui_window *parent) {
	l->h = gtk_label_new("");
	gtk_label_set_xalign(GTK_LABEL(l->h), 0);
	l->wnd = parent;
	return 0;
}

#define ffui_label_settextz(l, text)  gtk_label_set_text(GTK_LABEL((l)->h), text)
static inline void ffui_label_settext(ffui_label *l, const char *text, ffsize len) {
	char *sz = ffsz_dupn(text, len);
	ffui_label_settextz(l, sz);
	ffmem_free(sz);
}
#define ffui_label_settextstr(l, str)  ffui_label_settext(l, (str)->ptr, (str)->len)

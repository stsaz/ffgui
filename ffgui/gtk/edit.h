/** GUI/GTK+: edit
2019, Simon Zolin */

typedef struct ffui_edit ffui_edit;
struct ffui_edit {
	_FFUI_CTL_MEMBERS
	uint change_id;
};

FF_EXTERN void _ffui_edit_changed(GtkEditable *editable, gpointer udata);
static inline int ffui_edit_create(ffui_edit *e, ffui_window *parent) {
	e->h = gtk_entry_new();
	e->wnd = parent;
	g_signal_connect(e->h, "changed", (GCallback)_ffui_edit_changed, e);
	return 0;
}

#define ffui_edit_settextz(e, text)  gtk_entry_set_text(GTK_ENTRY((e)->h), text)
static inline void ffui_edit_settext(ffui_edit *e, const char *text, ffsize len) {
	if (len == 0) {
		ffui_edit_settextz(e, "");
		return;
	}
	char *sz = (text[len] != '\0') ? ffsz_dupn(text, len) : (char*)text;
	ffui_edit_settextz(e, sz);
	if (sz != text)
		ffmem_free(sz);
}
#define ffui_edit_settextstr(e, str)  ffui_edit_settext(e, (str)->ptr, (str)->len)

static inline ffstr ffui_edit_text(ffui_edit *e) {
	const gchar *sz = gtk_entry_get_text(GTK_ENTRY(e->h));
	ffstr s;
	s.len = ffsz_len(sz);
	s.ptr = ffsz_dupn(sz, s.len);
	return s;
}

#define ffui_edit_sel(e, start, end) \
	gtk_editable_select_region(GTK_EDITABLE((e)->h), start, end);

#define ffui_edit_selall(e) \
	gtk_editable_select_region(GTK_EDITABLE((e)->h), 0, -1);

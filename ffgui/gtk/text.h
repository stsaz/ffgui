/** GUI/GTK+: text
2019, Simon Zolin */

typedef struct ffui_text {
	_FFUI_CTL_MEMBERS
} ffui_text;

static inline int ffui_text_create(ffui_text *t, ffui_window *parent) {
	t->h = gtk_text_view_new();
	t->wnd = parent;
	return 0;
}

static inline void ffui_text_addtext(ffui_text *t, const char *text, ffsize len) {
	GtkTextBuffer *gbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(t->h));
	GtkTextIter end;
	gtk_text_buffer_get_end_iter(gbuf, &end);
	gtk_text_buffer_insert(gbuf, &end, text, len);
}
#define ffui_text_addtextz(t, text)  ffui_text_addtext(t, text, ffsz_len(text))
#define ffui_text_addtextstr(t, str)  ffui_text_addtext(t, (str)->ptr, (str)->len)

static inline ffstr ffui_text_text(ffui_text *t) {
	GtkTextBuffer *gbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(t->h));
	GtkTextIter end;
	gtk_text_buffer_get_end_iter(gbuf, &end);
	gchar *sz = gtk_text_buffer_get_text(gbuf, NULL, &end, 1);
	ffstr s;
	s.len = ffsz_len(sz);
	s.ptr = ffsz_dupn(sz, s.len);
	g_free(sz);
	return s;
}

static inline void ffui_text_clear(ffui_text *t) {
	GtkTextBuffer *gbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(t->h));
	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter(gbuf, &start);
	gtk_text_buffer_get_end_iter(gbuf, &end);
	gtk_text_buffer_delete(gbuf, &start, &end);
}

static inline void ffui_text_scroll(ffui_text *t, int pos) {
	GtkTextBuffer *gbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(t->h));
	GtkTextIter iter;
	if (pos == -1)
		gtk_text_buffer_get_end_iter(gbuf, &iter);
	else
		return;
	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(t->h), &iter, 0.0, 0, 0.0, 1.0);
}

/**
mode: GTK_WRAP_NONE GTK_WRAP_CHAR GTK_WRAP_WORD GTK_WRAP_WORD_CHAR*/
#define ffui_text_setwrapmode(t, mode)  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW((t)->h), mode)

#define ffui_text_setmonospace(t, mono)  gtk_text_view_set_monospace(GTK_TEXT_VIEW((t)->h), mono)

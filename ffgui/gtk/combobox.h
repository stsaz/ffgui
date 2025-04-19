/** GUI/GTK+: combobox
2019, Simon Zolin */

typedef struct ffui_combobox ffui_combobox;
struct ffui_combobox {
	_FFUI_CTL_MEMBERS
	uint change_id;
};

FF_EXTERN void _ffui_combo_changed(GtkComboBox *self, gpointer udata);
static inline void ffui_combo_create(ffui_combobox *cb, ffui_window *parent) {
	cb->h = gtk_combo_box_text_new();
	cb->wnd = parent;
	g_signal_connect(cb->h, "changed", (GCallback)_ffui_combo_changed, cb);
}

#define ffui_combo_set(c, i)  gtk_combo_box_set_active((GtkComboBox*)(c)->h, i)
#define ffui_combo_active(c)  gtk_combo_box_get_active((GtkComboBox*)(c)->h)

#define ffui_combo_add(c, text)  gtk_combo_box_text_append_text((GtkComboBoxText*)(c)->h, text)
#define ffui_combo_clear(c)  gtk_combo_box_text_remove_all((GtkComboBoxText*)(c)->h)

static inline ffstr ffui_combo_text_active(ffui_combobox *c) {
	gchar *sz = gtk_combo_box_text_get_active_text((GtkComboBoxText*)(c)->h);
	ffstr s;
	s.len = ffsz_len(sz);
	s.ptr = ffsz_dupn(sz, s.len);
	g_free(sz);
	return s;
}

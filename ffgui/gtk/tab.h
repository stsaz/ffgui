/** GUI/GTK+: tab
2019, Simon Zolin */

typedef struct ffui_tab ffui_tab;
struct ffui_tab {
	_FFUI_CTL_MEMBERS
	uint change_id;
	uint changed_index;
};

FF_EXTERN int ffui_tab_create(ffui_tab *t, ffui_window *parent);

FF_EXTERN void ffui_tab_ins(ffui_tab *t, int idx, const char *textz);

#define ffui_tab_append(t, textz)  ffui_tab_ins(t, -1, textz)

FF_EXTERN void _ffui_tab_switch_page(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer udata);

static inline void ffui_tab_del(ffui_tab *t, uint idx) {
	g_signal_handlers_block_by_func(t->h, (gpointer)_ffui_tab_switch_page, t);
	gtk_notebook_remove_page((GtkNotebook*)t->h, idx);
	g_signal_handlers_unblock_by_func(t->h, (gpointer)_ffui_tab_switch_page, t);
}

#define ffui_tab_count(t)  gtk_notebook_get_n_pages(GTK_NOTEBOOK((t)->h))

#define ffui_tab_active(t)  gtk_notebook_get_current_page(GTK_NOTEBOOK((t)->h))

FF_EXTERN void ffui_tab_setactive(ffui_tab *t, int idx);

#define ffui_tab_changed_index(t)  (t)->changed_index

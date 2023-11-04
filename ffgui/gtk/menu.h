/** GUI/GTK+: menu
2019, Simon Zolin */

typedef struct ffui_menu {
	_FFUI_CTL_MEMBERS
} ffui_menu;

static inline int ffui_menu_createmain(ffui_menu *m) {
	m->h = gtk_menu_bar_new();
	return (m->h == NULL);
}

static inline int ffui_menu_create(ffui_menu *m) {
	m->h = gtk_menu_new();
	return (m->h == NULL);
}

#define ffui_menu_new(text)  gtk_menu_item_new_with_mnemonic(text)
#define ffui_menu_newsep()  gtk_separator_menu_item_new()

static inline void ffui_menu_setsubmenu(void *mi, ffui_menu *sub, ffui_window *wnd) {
	gtk_menu_item_set_submenu((GtkMenuItem*)mi, sub->h);
	g_object_set_data(G_OBJECT(sub->h), "ffdata", wnd);
}

FF_EXTERN void _ffui_menu_activate(GtkWidget *mi, gpointer udata);
static inline void ffui_menu_setcmd(void *mi, uint id) {
	g_signal_connect(mi, "activate", (GCallback)_ffui_menu_activate, (void*)(ffsize)id);
}

static inline void ffui_menu_ins(ffui_menu *m, void *mi, int pos) {
	gtk_menu_shell_insert(GTK_MENU_SHELL(m->h), GTK_WIDGET(mi), pos);
}

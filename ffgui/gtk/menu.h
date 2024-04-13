/** GUI/GTK+: menu
2019, Simon Zolin */

#include <ffbase/vector.h>

struct _ffui_menu_item {
	void *mi;
	uint id;
};

typedef struct ffui_menu {
	_FFUI_CTL_MEMBERS
	ffvec items; // struct _ffui_menu_item[]
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
#define ffui_menu_check_new(text)  gtk_check_menu_item_new_with_mnemonic(text)
#define ffui_menu_separator_new()  gtk_separator_menu_item_new()

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

static inline void ffui_menu_store_id(ffui_menu *m, void *mi, uint id) {
	struct _ffui_menu_item *i = ffvec_pushT(&m->items, struct _ffui_menu_item);
	i->mi = mi;
	i->id = id;
}

static inline void ffui_menu_check(ffui_menu *m, uint id, int check) {
	struct _ffui_menu_item *it;
	FFSLICE_WALK(&m->items, it) {
		if (it->id == id) {
			g_signal_handlers_block_by_func(it->mi, (GClosure*)_ffui_menu_activate, (void*)(size_t)id);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(it->mi), check);
			g_signal_handlers_unblock_by_func(it->mi, (GClosure*)_ffui_menu_activate, (void*)(size_t)id);
			break;
		}
	}
}

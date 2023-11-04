/** GUI/GTK+: dialogs
2019, Simon Zolin */

#pragma once
#include <ffgui/gtk/gtk.h>

typedef struct ffui_dialog ffui_dialog;
struct ffui_dialog {
	char *title;
	char *name;
	GSList *names;
	GSList *curname;
	uint multisel :1;
};

static inline void ffui_dlg_init(ffui_dialog *d) {}

static inline void ffui_dlg_destroy(ffui_dialog *d) {
	g_slist_free_full(d->names, g_free);  d->names = NULL;
	ffmem_free(d->title); d->title = NULL;
	g_free(d->name); d->name = NULL;
}

static inline void ffui_dlg_titlez(ffui_dialog *d, const char *sz) {
	d->title = ffsz_dup(sz);
}

#define ffui_dlg_multisel(d, val)  ((d)->multisel = (val))

/** Get the next file name (for a dialog with multiselect). */
static inline char* ffui_dlg_nextname(ffui_dialog *d) {
	if (d->curname == NULL)
		return NULL;
	char *name = (char*)d->curname->data;
	d->curname = d->curname->next;
	return name;
}

FF_EXTERN char* ffui_dlg_open(ffui_dialog *d, ffui_window *parent);

FF_EXTERN char* ffui_dlg_save(ffui_dialog *d, ffui_window *parent, const char *fn, ffsize fnlen);

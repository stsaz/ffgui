/** GUI-winapi: combobox
2014, Simon Zolin */

typedef struct ffui_combobox {
	HWND h;
	enum FFUI_UID uid;
	const char *name;
	HFONT font;
	uint change_id;
	uint popup_id;
	uint closeup_id;
	uint edit_change_id;
	uint edit_update_id;
} ffui_combobox;

FF_EXTERN int ffui_combo_create(ffui_ctl *c, ffui_window *parent);
FF_EXTERN int ffui_combo_createlist(ffui_ctl *c, ffui_window *parent);

/** Insert an item
idx: -1: insert to end */
static inline void ffui_combo_insert(ffui_combobox *c, int idx, const char *txt, ffsize len) {
	wchar_t *w, ws[255];
	ffsize n = FF_COUNT(ws) - 1;
	if (NULL == (w = ffs_utow(ws, &n, txt, len)))
		return;
	w[n] = '\0';
	uint msg = CB_INSERTSTRING;
	if (idx == -1) {
		idx = 0;
		msg = CB_ADDSTRING;
	}
	ffui_ctl_send(c, msg, idx, w);
	if (w != ws)
		ffmem_free(w);
}
#define ffui_combo_insz(c, idx, textz)  ffui_combo_insert(c, idx, textz, ffsz_len(textz))
#define ffui_combo_add(c, textz)  ffui_combo_insert(c, -1, textz, ffsz_len(textz))

/** Remove item */
#define ffui_combo_remove(c, idx)  ffui_ctl_send(c, CB_DELETESTRING, idx, 0)

/** Remove all items */
#define ffui_combo_clear(c)  ffui_ctl_send(c, CB_RESETCONTENT, 0, 0)

/** Get number of items */
#define ffui_combo_count(c)  ((uint)ffui_ctl_send(c, CB_GETCOUNT, 0, 0))

/** Set/get active index */
#define ffui_combo_set(c, idx)  ffui_ctl_send(c, CB_SETCURSEL, idx, 0)
#define ffui_combo_active(c)  ((uint)ffui_ctl_send(c, CB_GETCURSEL, 0, 0))

/** Get text */
FF_EXTERN ffstr ffui_combo_text(ffui_combobox *c, uint idx);
#define ffui_combo_text_active(c)  ffui_combo_text(c, ffui_combo_active(c))

/** Show/hide drop down list */
#define ffui_combo_popup(c, show)  ffui_ctl_send(c, CB_SHOWDROPDOWN, show, 0)

/** GUI-winapi: editbox
2014, Simon Zolin */

#pragma once
#include <ffgui/winapi/winapi.h>

typedef struct ffui_edit {
	HWND h;
	enum FFUI_UID uid;
	const char *name;
	HFONT font;
	uint change_id;
	uint focus_id, focus_lose_id;
} ffui_edit;
typedef ffui_edit ffui_text;

FF_EXTERN int ffui_edit_create(ffui_ctl *c, ffui_window *parent);
FF_EXTERN int ffui_text_create(ffui_ctl *c, ffui_window *parent);

static inline ffstr ffui_edit_text(ffui_edit *e) {
	ffstr s = {};
	ffui_textstr(e, &s);
	return s;
}

#define ffui_edit_settextz(c, sz)  ffui_settext(c, sz, ffsz_len(sz))
#define ffui_edit_settextstr(e, str)  ffui_settext(e, (str)->ptr, (str)->len)

#define ffui_edit_password(e, enable) \
	ffui_ctl_send(e, EM_SETPASSWORDCHAR, (enable) ? (wchar_t)0x25CF : 0, 0)

#define ffui_edit_readonly(e, val) \
	ffui_ctl_send(e, EM_SETREADONLY, val, 0)

FF_EXTERN int ffui_edit_addtext(ffui_edit *c, const char *text, size_t len);
#define ffui_text_addtextstr(t, s)  ffui_edit_addtext(t, (s)->ptr, (s)->len)

#define ffui_edit_sel(e, off, n)  ffui_send((e)->h, EM_SETSEL, off, (off) + (n))
#define ffui_edit_selall(e)  ffui_send((e)->h, EM_SETSEL, 0, -1)

/** Get the number of lines */
#define ffui_edit_countlines(e)  ffui_send((e)->h, EM_GETLINECOUNT, 0, 0)

/** Get character index under the specified coordinates */
#define ffui_edit_charfrompos(e, pt) \
	ffui_send((e)->h, EM_CHARFROMPOS, 0, MAKELPARAM((pt)->x, (pt)->y))

enum FFUI_EDIT_SCROLL {
	FFUI_EDIT_SCROLL_LINEUP, FFUI_EDIT_SCROLL_LINEDOWN,
	FFUI_EDIT_SCROLL_PAGEUP, FFUI_EDIT_SCROLL_PAGEDOWN,
};
/*
type: enum FFUI_EDIT_SCROLL */
#define ffui_edit_scroll(e, type)  ffui_send((e)->h, EM_SCROLL, type, 0)

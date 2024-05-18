/** GUI-winapi: window
2014, Simon Zolin */

#pragma once
#include <ffgui/winapi/winapi.h>

struct ffui_window {
	HWND h;
	enum FFUI_UID uid;
	const char *name;
	HFONT font;
	HBRUSH bgcolor;
	uint color; // default color for child controls
	uint top :1 //quit message loop if the window is closed
		, hide_on_close :1 //window doesn't get destroyed when it's closed
		, manual_close :1 //don't automatically close window on X button press
		, popup :1;
	byte bordstick; //stick window to screen borders

	HWND ttip;
	HWND focused; //restore focus when the window is activated again
	ffui_trayicon *trayicon;
	ffui_paned *paned_first;
	ffui_statusbar *stbar;
	HACCEL acceltbl;
	ffvec ghotkeys; //struct wnd_ghotkey[]

	void (*on_create)(ffui_window *wnd);
	void (*on_destroy)(ffui_window *wnd);
	void (*on_action)(ffui_window *wnd, int id);
	void (*on_dropfiles)(ffui_window *wnd, ffui_fdrop *df);

	/** WM_PAINT handler. */
	void (*on_paint)(ffui_window *wnd);

	uint onclose_id;
	uint onminimize_id;
	uint onmaximize_id;
	uint onactivate_id;
};

FF_EXTERN int ffui_wnd_initstyle();

static inline void ffui_wnd_setpopup(ffui_window *w) {
	LONG st = GetWindowLongW(w->h, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW;
	SetWindowLongW(w->h, GWL_STYLE, st | WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME);
	w->popup = 1;
}

enum {
	FFUI_WM_USER_TRAY = WM_USER + 1000
};

FF_EXTERN int ffui_wndproc(ffui_window *wnd, ffsize *code, HWND h, uint msg, ffsize w, ffsize l);

FF_EXTERN int ffui_wnd_create(ffui_window *w);

#define ffui_desktop(w)  (w)->h = GetDesktopWindow()

#define ffui_send_wnd_settext(c, sz)  ffui_settext(c, sz, ffsz_len(sz))

#define ffui_wnd_front(w)  (w)->h = GetForegroundWindow()
#define ffui_wnd_setfront(w)  SetForegroundWindow((w)->h)

static inline void ffui_wnd_icon(ffui_window *w, ffui_icon *big_ico, ffui_icon *small_ico) {
	big_ico->h = (HICON)ffui_ctl_send(w, WM_GETICON, ICON_BIG, 0);
	small_ico->h = (HICON)ffui_ctl_send(w, WM_GETICON, ICON_SMALL, 0);
}

static inline void ffui_wnd_seticon(ffui_window *w, const ffui_icon *big_ico, const ffui_icon *small_ico) {
	ffui_ctl_send(w, WM_SETICON, ICON_SMALL, small_ico->h);
	ffui_ctl_send(w, WM_SETICON, ICON_BIG, big_ico->h);
}

/** Set background color. */
static inline void ffui_wnd_bgcolor(ffui_window *w, uint color) {
	if (w->bgcolor != NULL)
		DeleteObject(w->bgcolor);
	w->bgcolor = CreateSolidBrush(color);
}

FF_EXTERN void ffui_wnd_opacity(ffui_window *w, uint percent);

#define ffui_wnd_close(w)  ffui_ctl_send(w, WM_CLOSE, 0, 0)

FF_EXTERN int ffui_wnd_destroy(ffui_window *w);

#define ffui_wnd_pos(w, pos) \
	ffui_getpos2(w, pos, FFUI_FPOS_DPISCALE)

/** Get window placement.
Return SW_*. */
static inline uint ffui_wnd_placement(ffui_window *w, ffui_pos *pos) {
	WINDOWPLACEMENT pl = {};
	pl.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(w->h, &pl);
	ffui_pos_fromrect(pos, &pl.rcNormalPosition);
	if (pl.showCmd == SW_SHOWNORMAL && !IsWindowVisible(w->h))
		pl.showCmd = SW_HIDE;
	return pl.showCmd;
}

static inline void ffui_wnd_setplacement(ffui_window *w, uint showcmd, const ffui_pos *pos) {
	WINDOWPLACEMENT pl = {};
	pl.length = sizeof(WINDOWPLACEMENT);
	pl.showCmd = showcmd;
	ffui_pos_torect(pos, &pl.rcNormalPosition);
	SetWindowPlacement(w->h, &pl);
}
#define ffui_post_wnd_place(w, showcmd, pos)  ffui_wnd_setplacement(w, showcmd, pos)

FF_EXTERN int ffui_wnd_tooltip(ffui_window *w, ffui_ctl *ctl, const char *text, ffsize len);

typedef struct ffui_wnd_hotkey {
	uint hk;
	uint cmd;
} ffui_wnd_hotkey;

/** Set hotkey table. */
FF_EXTERN int ffui_wnd_hotkeys(ffui_window *w, const struct ffui_wnd_hotkey *hotkeys, ffsize n);

/** Register a global hotkey. */
FF_EXTERN int ffui_wnd_ghotkey_reg(ffui_window *w, uint hk, uint cmd);

/** Unregister all global hotkeys associated with this window. */
FF_EXTERN void ffui_wnd_ghotkey_unreg(ffui_window *w);

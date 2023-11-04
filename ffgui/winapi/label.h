/** GUI-winapi: label
2014, Simon Zolin */

typedef struct ffui_label {
	FFUI_CTL;
	HFONT font;
	HCURSOR cursor;
	uint color;
	uint click_id;
	uint click2_id;
	WNDPROC oldwndproc;
} ffui_label;

FF_EXTERN int ffui_label_create(ffui_label *c, ffui_window *parent);

#define ffui_send_label_settext(c, sz)  ffui_settextz(c, sz)

/**
type: enum FFUI_CUR */
static inline void ffui_label_setcursor(ffui_label *c, uint type) {
	ffui_ctl_setcursor(c, LoadCursorW(NULL, (wchar_t*)(size_t)type));
}

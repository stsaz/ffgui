/** GUI-winapi: label
2014, Simon Zolin */

typedef struct ffui_label {
	FFUI_CTL;
	HFONT font;
	HCURSOR cursor;
	uint color;
	ushort click_id, click2_id;
} ffui_label;

static inline int ffui_label_create(ffui_label *c, ffui_window *parent) {
	return _ffui_ctl_create_inherit_font(c, FFUI_UID_LABEL, parent);
}

#define ffui_send_label_settext(c, sz)  ffui_settextz(c, sz)

/**
type: enum FFUI_CUR */
FF_EXTERN void ffui_label_setcursor(ffui_label *c, uint type);

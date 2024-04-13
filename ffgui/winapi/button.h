/** GUI-winapi: button
2014, Simon Zolin */

struct ffui_button {
	FFUI_CTL;
	HFONT font;
	uint action_id;
};

static inline int ffui_button_create(ffui_ctl *c, ffui_window *parent) {
	return _ffui_ctl_create_inherit_font(c, FFUI_UID_BUTTON, parent);
}

#define ffui_button_settextz(c, sz)  ffui_settextz(c, sz)

static inline void ffui_button_seticon(ffui_button *b, const ffui_icon *ico) {
	_ffui_style_set(b, BS_ICON);
	ffui_ctl_send(b, BM_SETIMAGE, IMAGE_ICON, ico->h);
}
#define ffui_send_button_seticon  ffui_button_seticon

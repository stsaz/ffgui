/** GUI-winapi: button
2014, Simon Zolin */

struct ffui_button {
	FFUI_CTL;
	HFONT font;
	uint action_id;
};

FF_EXTERN int ffui_button_create(ffui_ctl *c, ffui_window *parent);

#define ffui_button_settextz(c, sz)  ffui_settextz(c, sz)

static inline void ffui_button_seticon(ffui_button *b, ffui_icon *ico) {
	_ffui_style_set(b, BS_ICON);
	ffui_ctl_send(b, BM_SETIMAGE, IMAGE_ICON, ico->h);
}

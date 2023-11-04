/** GUI-winapi: radiobutton
2014, Simon Zolin */

struct ffui_radio {
	FFUI_CTL;
	HFONT font;
	uint action_id;
};

FF_EXTERN int ffui_radio_create(ffui_ctl *c, ffui_window *parent);

#define ffui_radio_check(c)  ffui_checkbox_check(c)
#define ffui_radio_checked(c)  ffui_checkbox_checked(c)

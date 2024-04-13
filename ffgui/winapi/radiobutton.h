/** GUI-winapi: radiobutton
2014, Simon Zolin */

struct ffui_radio {
	FFUI_CTL;
	HFONT font;
	uint action_id;
};

static inline int ffui_radio_create(ffui_ctl *c, ffui_window *parent) {
	return _ffui_ctl_create_inherit_font(c, FFUI_UID_RADIO, parent);
}

#define ffui_radio_check(c)  ffui_checkbox_check(c)
#define ffui_radio_checked(c)  ffui_checkbox_checked(c)

/** GUI-winapi: checkbox
2014, Simon Zolin */

typedef struct ffui_checkbox {
	FFUI_CTL;
	HFONT font;
	uint action_id;
} ffui_checkbox;

FF_EXTERN int ffui_checkbox_create(ffui_ctl *c, ffui_window *parent);

#define ffui_checkbox_check(c, val)  ffui_ctl_send(c, BM_SETCHECK, val, 0)
#define ffui_checkbox_checked(c)  ffui_ctl_send(c, BM_GETCHECK, 0, 0)

#define ffui_checkbox_check(c, val)  ffui_ctl_send(c, BM_SETCHECK, val, 0)
#define ffui_checkbox_checked(c)  ffui_ctl_send(c, BM_GETCHECK, 0, 0)
#define ffui_send_checkbox_checked(c)  ffui_ctl_send(c, BM_GETCHECK, 0, 0)

#define ffui_send_checkbox_settextz(c, sz)  ffui_settextz(c, sz)

/** GUI-winapi: progressbar
2014, Simon Zolin */

FF_EXTERN int ffui_progress_create(ffui_ctl *c, ffui_window *parent);

#define ffui_progress_set(p, val) \
	ffui_ctl_send(p, PBM_SETPOS, val, 0)

#define ffui_progress_setrange(p, max) \
	ffui_ctl_send(p, PBM_SETRANGE, 0, MAKELPARAM(0, max))

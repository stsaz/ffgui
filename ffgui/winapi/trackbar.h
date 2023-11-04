/** GUI-winapi: trackbar
2014, Simon Zolin */

typedef struct ffui_trackbar {
	FFUI_CTL;
	uint scroll_id;
	uint scrolling_id;
	uint thumbtrk :1; //prevent trackbar from updating position while user's holding it
} ffui_trackbar;

FF_EXTERN int ffui_track_create(ffui_trackbar *t, ffui_window *parent);

#define ffui_track_setrange(t, max)  ffui_ctl_send(t, TBM_SETRANGEMAX, 1, max)
#define ffui_post_track_range_set(t, max)  ffui_ctl_send(t, TBM_SETRANGEMAX, 1, max)

#define ffui_track_setpage(t, pagesize)  ffui_ctl_send(t, TBM_SETPAGESIZE, 0, pagesize)

static inline void ffui_track_set(ffui_trackbar *t, uint val) {
	if (!t->thumbtrk)
		ffui_ctl_send(t, TBM_SETPOS, 1, val);
}
#define ffui_post_track_set(t, val)  ffui_track_set(t, val)

#define ffui_track_val(t)  ffui_ctl_send(t, TBM_GETPOS, 0, 0)

#define ffui_track_setdelta(t, delta) \
	ffui_track_set(t, (int)ffui_track_val(t) + delta)

enum FFUI_TRK_MOVE {
	FFUI_TRK_PGUP,
	FFUI_TRK_PGDN,
};

/**
cmd: enum FFUI_TRK_MOVE */
static inline void ffui_track_move(ffui_trackbar *t, uint cmd) {
	uint pgsize = ffui_ctl_send(t, TBM_GETPAGESIZE, 0, 0);
	uint pos = ffui_track_val(t);
	switch (cmd) {
	case FFUI_TRK_PGUP:
		pos += pgsize;
		break;
	case FFUI_TRK_PGDN:
		pos -= pgsize;
		break;
	}
	ffui_track_set(t, pos);
}

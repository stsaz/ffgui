/** GUI-winapi: statusbar
2014, Simon Zolin */

typedef struct ffui_statusbar ffui_statusbar;
struct ffui_statusbar {
	FFUI_CTL;
};

FF_EXTERN int ffui_status_create(ffui_statusbar *c, ffui_window *parent);

#define ffui_status_setparts(sb, n, parts)  ffui_send((sb)->h, SB_SETPARTS, n, parts)

static inline void ffui_status_settext(ffui_statusbar *sb, int idx, const char *text, ffsize len) {
	wchar_t *w, ws[255];
	ffsize n = FF_COUNT(ws) - 1;
	if (NULL == (w = ffs_utow(ws, &n, text, len)))
		return;
	w[n] = '\0';
	ffui_send(sb->h, SB_SETTEXT, idx, w);
	if (w != ws)
		ffmem_free(w);
}
#define ffui_status_settextstr(sb, idx, str)  ffui_status_settext(sb, idx, (str)->ptr, (str)->len)
#define ffui_status_settextz(sb, idx, sz)  ffui_status_settext(sb, idx, sz, ffsz_len(sz))
#define ffui_send_status_settextz(sb, sz)  ffui_settext(sb, sz, ffsz_len(sz))

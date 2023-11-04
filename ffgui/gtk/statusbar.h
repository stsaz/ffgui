/** GUI/GTK+: statusbar
2019, Simon Zolin */

typedef struct ffui_statusbar {
	_FFUI_CTL_MEMBERS
} ffui_statusbar;

FF_EXTERN int ffui_status_create(ffui_ctl *sb, ffui_window *parent);

static inline void ffui_status_settextz(ffui_ctl *sb, const char *text) {
	gtk_statusbar_push(GTK_STATUSBAR(sb->h), gtk_statusbar_get_context_id(GTK_STATUSBAR(sb->h), "a"), text);
}

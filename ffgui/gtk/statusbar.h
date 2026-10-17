/** GUI/GTK+: statusbar
2019, Simon Zolin */

struct ffui_statusbar {
	_FFUI_CTL_MEMBERS
	GtkWidget* parts[2];
};

FF_EXTERN int ffui_status_create(ffui_statusbar *sb, ffui_window *parent);

FF_EXTERN void ffui_status_setparts(ffui_statusbar *sb, uint n);

static inline void ffui_status_settextz(ffui_statusbar *sb, uint i, const char *text) {
	FF_ASSERT(i < FF_COUNT(sb->parts));
	gtk_label_set_text(GTK_LABEL(sb->parts[i]), text);
}

/** GUI/GTK+: trayicon
2019, Simon Zolin */

typedef struct ffui_trayicon {
	_FFUI_CTL_MEMBERS
	uint lclick_id;
} ffui_trayicon;

FF_EXTERN int ffui_tray_create(ffui_trayicon *t, ffui_window *wnd);

#define ffui_tray_hasicon(t) \
	(0 != gtk_status_icon_get_size(GTK_STATUS_ICON((t)->h)))

static inline void ffui_tray_seticon(ffui_trayicon *t, ffui_icon *ico) {
	gtk_status_icon_set_from_pixbuf(GTK_STATUS_ICON(t->h), ico->ico);
}

#define ffui_tray_show(t, show)  gtk_status_icon_set_visible(GTK_STATUS_ICON((t)->h), show)

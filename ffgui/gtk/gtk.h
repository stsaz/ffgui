/** GUI based on GTK+.
2019, Simon Zolin */

#pragma once
#include <ffbase/string.h>
#include <ffbase/stringz.h>
#include <gtk/gtk.h>

#ifdef FFGUI_DEBUG
	#include <ffsys/std.h>
	#define _ffui_log fflog
#else
	#define _ffui_log(...)
#endif

static inline void ffui_init() {
	int argc = 0;
	char **argv = NULL;
	gtk_init(&argc, &argv);
}

#define ffui_uninit()


typedef struct ffui_pos {
	int x, y
		, cx, cy;
} ffui_pos;


// ICON
typedef struct ffui_icon {
	GdkPixbuf *ico;
} ffui_icon;

static inline int ffui_icon_load(ffui_icon *ico, const char *filename) {
	ico->ico = gdk_pixbuf_new_from_file(filename, NULL);
	return (ico->ico == NULL);
}

static inline int ffui_icon_loadimg(ffui_icon *ico, const char *filename, uint cx, uint cy, uint flags) {
	ico->ico = gdk_pixbuf_new_from_file_at_scale(filename, cx, cy, 0, NULL);
	return (ico->ico == NULL);
}


// CONTROL

enum FFUI_UID {
	FFUI_UID_WINDOW = 1,
	FFUI_UID_TRACKBAR,
};

typedef struct ffui_window ffui_window;

#define _FFUI_CTL_MEMBERS \
	GtkWidget *h; \
	enum FFUI_UID uid; \
	ffui_window *wnd;

typedef struct ffui_ctl {
	_FFUI_CTL_MEMBERS
} ffui_ctl;

typedef struct ffui_view ffui_view;

static inline void ffui_show(void *c, uint show) {
	if (show)
		gtk_widget_show_all(((ffui_ctl*)c)->h);
	else
		gtk_widget_hide(((ffui_ctl*)c)->h);
}

#define ffui_ctl_focus(c)  gtk_widget_grab_focus((c)->h)

#define ffui_ctl_enable(c, val)  ffui_post(c, FFUI_CTL_ENABLE, (void*)(ffsize)val)

#define ffui_ctl_destroy(c)  gtk_widget_destroy(((ffui_ctl*)c)->h)

#define ffui_setposrect(ctl, r) \
	gtk_widget_set_size_request((ctl)->h, (r)->cx, (r)->cy)

// MESSAGE LOOP
FF_EXTERN void ffui_run();

#define ffui_quitloop()  gtk_main_quit()

typedef void (*ffui_handler)(void *param);

FF_EXTERN void ffui_thd_post(ffui_handler func, void *udata);

typedef struct ffui_tab ffui_tab;

enum FFUI_MSG {
	FFUI_QUITLOOP,
	FFUI_CHECKBOX_SETTEXTZ,
	FFUI_CHECKBOX_CHECKED,
	FFUI_CLIP_SETTEXT,
	FFUI_EDIT_GETTEXT,
	FFUI_LBL_SETTEXT,
	FFUI_STBAR_SETTEXT,
	FFUI_TAB_ACTIVE,
	FFUI_TAB_COUNT,
	FFUI_TAB_INS,
	FFUI_TAB_SETACTIVE,
	FFUI_TEXT_ADDTEXT,
	FFUI_TEXT_SETTEXT,
	FFUI_CTL_ENABLE,
	FFUI_TRK_SET,
	FFUI_TRK_SETRANGE,
	FFUI_VIEW_CLEAR,
	FFUI_VIEW_SCROLL,
	FFUI_VIEW_SCROLLSET,
	FFUI_VIEW_SETDATA,
	FFUI_VIEW_SEL_SINGLE,
	FFUI_WND_SETTEXT,
	FFUI_WND_SHOW,
};

/**
id: enum FFUI_MSG */
FF_EXTERN void ffui_post(void *ctl, uint id, void *udata);
FF_EXTERN ffsize ffui_send(void *ctl, uint id, void *udata);

#define ffui_post_quitloop()  ffui_post(NULL, FFUI_QUITLOOP, NULL)
#define ffui_send_label_settext(ctl, sz)  ffui_send(ctl, FFUI_LBL_SETTEXT, (void*)sz)
#define ffui_send_edit_textstr(ctl, str_dst)  ffui_send(ctl, FFUI_EDIT_GETTEXT, str_dst)
#define ffui_send_text_settextstr(ctl, str)  ffui_send(ctl, FFUI_TEXT_SETTEXT, (void*)str)
#define ffui_send_text_addtextstr(ctl, str)  ffui_send(ctl, FFUI_TEXT_ADDTEXT, (void*)str)
#define ffui_send_checkbox_settextz(ctl, sz)  ffui_send(ctl, FFUI_CHECKBOX_SETTEXTZ, (void*)sz)
#define ffui_send_wnd_settext(ctl, sz)  ffui_send(ctl, FFUI_WND_SETTEXT, (void*)sz)
#define ffui_post_wnd_show(ctl, show)  ffui_send(ctl, FFUI_WND_SHOW, (void*)(ffsize)show)
#define ffui_post_view_clear(ctl)  ffui_post(ctl, FFUI_VIEW_CLEAR, NULL)

#define ffui_send_view_scroll(c) ({ \
	ffsize val; \
	ffui_send(c, FFUI_VIEW_SCROLL, &val); \
	val; \
})

#define ffui_post_view_scroll_set(ctl, vert_pos)  ffui_post(ctl, FFUI_VIEW_SCROLLSET, (void*)(ffsize)vert_pos)
#define ffui_clipboard_settextstr(str)  ffui_send(NULL, FFUI_CLIP_SETTEXT, (void*)str)

#define ffui_send_checkbox_checked(c) ({ \
	ffsize val; \
	ffui_send(c, FFUI_CHECKBOX_CHECKED, &val); \
	!!val; \
})

static inline void ffui_send_view_setdata(ffui_view *v, uint first, int delta) {
	ffsize p = ((first & 0xffff) << 16) | (delta & 0xffff);
	ffui_send(v, FFUI_VIEW_SETDATA, (void*)p);
}
static inline void ffui_post_view_setdata(ffui_view *v, uint first, int delta) {
	ffsize p = ((first & 0xffff) << 16) | (delta & 0xffff);
	ffui_post(v, FFUI_VIEW_SETDATA, (void*)p);
}

#define ffui_post_view_select_single(v, pos)  ffui_post(v, FFUI_VIEW_SEL_SINGLE, (void*)(size_t)pos)

#define ffui_post_track_range_set(ctl, range)  ffui_post(ctl, FFUI_TRK_SETRANGE, (void*)(ffsize)range)
#define ffui_post_track_set(ctl, val)  ffui_post(ctl, FFUI_TRK_SET, (void*)(ffsize)val)

#define ffui_send_tab_ins(ctl, textz)  ffui_send(ctl, FFUI_TAB_INS, (void*)textz)
#define ffui_send_tab_setactive(ctl, idx)  ffui_send(ctl, FFUI_TAB_SETACTIVE, (void*)(ffsize)idx)
static inline int ffui_send_tab_active(ffui_tab *ctl) {
	ffsize idx;
	ffui_send(ctl, FFUI_TAB_ACTIVE, &idx);
	return idx;
}
static inline int ffui_send_tab_count(ffui_tab *ctl) {
	ffsize n;
	ffui_send(ctl, FFUI_TAB_COUNT, &n);
	return n;
}

#define ffui_send_status_settextz(sb, sz)  ffui_send(sb, FFUI_STBAR_SETTEXT, (void*)sz)

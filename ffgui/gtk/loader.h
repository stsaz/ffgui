/** GUI based on GTK+
2019, Simon Zolin */

#pragma once
#include <ffgui/gtk/gtk.h>
#include <ffgui/gtk/button.h>
#include <ffgui/gtk/checkbox.h>
#include <ffgui/gtk/combobox.h>
#include <ffgui/gtk/dialog.h>
#include <ffgui/gtk/edit.h>
#include <ffgui/gtk/image.h>
#include <ffgui/gtk/label.h>
#include <ffgui/gtk/menu.h>
#include <ffgui/gtk/statusbar.h>
#include <ffgui/gtk/tab.h>
#include <ffgui/gtk/text.h>
#include <ffgui/gtk/trackbar.h>
#include <ffgui/gtk/trayicon.h>
#include <ffgui/gtk/view.h>
#include <ffgui/gtk/window.h>
#include <ffgui/vars.h>
#include <ffbase/vector.h>

typedef void* (*ffui_ldr_getctl_t)(void *udata, const ffstr *name);

/** Get command ID by its name.
Return 0 if not found. */
typedef int (*ffui_ldr_getcmd_t)(void *udata, const ffstr *name);

typedef struct {
	char *fn;
} _ffui_ldr_icon;

typedef struct ffui_loader {
	ffui_ldr_getctl_t getctl;
	ffui_ldr_getcmd_t getcmd;
	void *udata;
	ffstr path;
	ffvec accels; //ffui_wnd_hotkey[]

	char language[2];
	ffvec lang_data_def, lang_data;
	ffmap vars; // hash(name) -> struct var*

	_ffui_ldr_icon ico;
	_ffui_ldr_icon ico_ctl;
	ffui_pos r;
	ffui_window *wnd;
	ffui_viewcol vicol;
	ffui_menu *menu;
	void *mi;
	GtkWidget *hbox;
	uint list_idx;
	union {
		ffui_button*	btn;
		ffui_checkbox*	cb;
		ffui_combobox*	combo;
		ffui_ctl*		ctl;
		ffui_dialog*	dlg;
		ffui_edit*		edit;
		ffui_image*		img;
		ffui_label*		lbl;
		ffui_tab*		tab;
		ffui_text*		text;
		ffui_trackbar*	trkbar;
		ffui_trayicon*	trayicon;
		ffui_view*		view;
	};

	char *errstr;
	char *wndname;

	union {
	uint flags;
	struct {
		uint f_horiz :1;
		uint f_loadconf :1; // ffui_ldr_loadconf()
	};
	};
} ffui_loader;

/** Initialize GUI loader.
getctl: get a pointer to a UI element by its name.
 Most of the time you just need to call ffui_ldr_findctl() from it.
getcmd: get command ID by its name
udata: user data */
FF_EXTERN void ffui_ldr_init(ffui_loader *g, ffui_ldr_getctl_t getctl, ffui_ldr_getcmd_t getcmd, void *udata);

FF_EXTERN void ffui_ldr_fin(ffui_loader *g);

#define ffui_ldr_errstr(g)  ((g)->errstr)

/** Load GUI from file. */
FF_EXTERN int ffui_ldr_loadfile(ffui_loader *g, const char *fn);

/** Apply config data.
Format:
	(ctx.key val CRLF)... */
FF_EXTERN void ffui_ldr_loadconf(ffui_loader *g, ffstr data);

/** GUI/GTK+: loader
2019, Simon Zolin */

#include <ffsys/error.h>
#include <ffgui/gtk/loader.h>
#include <ffgui/conf-scheme.h>
#include <ffsys/path.h>
#include <ffsys/file.h>

enum {
	FFUI_EINVAL = 100,
	FFUI_ENOMEM,
};

#define _F(x)  (ffsize)x
#define T_STR  FFCONF_TSTR
#define T_STRMULTI  (FFCONF_TSTR | FFCONF_FMULTI)
#define T_CLOSE  FFCONF_TCLOSE
#define T_OBJ  FFCONF_TOBJ
#define T_OBJ_ARG  (FFCONF_TOBJ | FFCONF_FARG)
#define T_INT32  FFCONF_TINT32
#define T_INTLIST  (FFCONF_TINT32 | FFCONF_FLIST)
#define T_STRLIST  (FFCONF_TSTR | FFCONF_FLIST)
#define T_OBJS_ARG (FFCONF_TOBJ | FFCONF_FARG | FFCONF_FMULTI)

static void add_ctx(ffui_loader *g, const ffconf_arg *args)
{
	ffconf_scheme_addctx(g->cs, args, g);
}

static void* ldr_getctl(ffui_loader *g, ffstr name)
{
	char buf[255];
	ffstr s;
	s.ptr = buf;
	s.len = ffs_format_r0(buf, sizeof(buf), "%s.%S", g->wnd_name, &name);
	return g->getctl(g->udata, &s);
}

static void ctl_reset(ffui_loader *g)
{
	g->f_horiz = 0;
	g->f_cbx_editable = 0;
}

enum CTL_PLACE {
	CTL_PLACE_EXPAND = 1,
	CTL_PLACE_FILL = 2,
	CTL_PLACE_EXPAND_H = 4,
};

static void ctl_place_f(ffui_loader *g, ffui_ctl *ctl, uint flags)
{
	if (g->f_horiz || g->f_horiz_prev) {
		if (g->hbox == NULL) {
			g->hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
			gtk_box_pack_start(GTK_BOX(g->wnd->vbox), g->hbox, !!(flags & CTL_PLACE_EXPAND_H), /*fill=*/0, /*padding=*/0);
		}
		gtk_box_pack_start(GTK_BOX(g->hbox), ctl->h, !!(flags & CTL_PLACE_EXPAND), !!(flags & CTL_PLACE_FILL), /*padding=*/0);
		g->f_horiz_prev = g->f_horiz;
	} else {
		gtk_box_pack_start(GTK_BOX(g->wnd->vbox), ctl->h, /*expand=*/0, /*fill=*/0, /*padding=*/0);
	}

	if (!g->f_horiz)
		g->hbox = NULL;
}

static void ctl_place(ffui_ctl *ctl, ffui_loader *g)
{
	ctl_place_f(g, ctl, 0);
}

static int ctl_size(ffui_loader *g, ffint64 val) { return 0; }
static int ctl_tooltip(ffui_loader *g, ffstr val) { return 0; }

// ICON
// MENU ITEM
// MENU
// LABEL
// IMAGE
// BUTTON
// CHECKBOX
// EDITBOX
// TEXT
// TRACKBAR
// TAB
// VIEW COL
// VIEW
// STATUSBAR
// TRAYICON
// DIALOG
// WINDOW

// ICON
static const ffconf_arg icon_args[] = {
	{ "filename",	FFCONF_TSTRZ, FF_OFF(_ffui_ldr_icon, fn) },
	{}
};
static int icon_new(ffui_loader *g)
{
	ffmem_zero_obj(&g->ico_ctl);
	ffconf_scheme_addctx(g->cs, icon_args, &g->ico_ctl);
	return 0;
}
static int icon_done(ffui_loader *g, ffui_icon *icon)
{
	if (g->ico_ctl.fn == NULL) return 0;

	char *sz = ffsz_allocfmt("%S/%s", &g->path, g->ico_ctl.fn);
	ffui_icon_load(icon, sz);
	ffmem_free(sz);
	ffmem_free(g->ico_ctl.fn);
	ffmem_zero_obj(&g->ico_ctl);
	return 1;
}


// MENU ITEM
static int mi_submenu(ffui_loader *g, ffstr val)
{
	ffui_menu *sub;
	if (!(sub = g->getctl(g->udata, &val)))
		return FFUI_EINVAL;

	ffui_menu_setsubmenu(g->mi, sub, g->wnd);
	return 0;
}

static int mi_action(ffui_loader *g, ffstr val)
{
	int id;
	if (!(id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;

	ffui_menu_setcmd(g->mi, id);
	ffui_menu_store_id(g->menu, g->mi, id);
	return 0;
}

static int mi_hotkey(ffui_loader *g, ffstr val)
{
	ffui_wnd_hotkey *a;
	uint hk;

	if (!(a = ffvec_pushT(&g->accels, ffui_wnd_hotkey)))
		return FFUI_ENOMEM;

	if (!(hk = ffui_hotkey_parse(val.ptr, val.len)))
		return FFUI_EINVAL;

	a->hk = hk;
	a->h = g->mi;
	return 0;
}

static int mi_done(ffui_loader *g)
{
	ffui_menu_ins(g->menu, g->mi, -1);
	return 0;
}

static const ffconf_arg mi_args[] = {
	{ "action",		T_STR,		_F(mi_action) },
	{ "hotkey",		T_STR,		_F(mi_hotkey) },
	{ "submenu",	T_STR,		_F(mi_submenu) },
	{ NULL,			T_CLOSE,	_F(mi_done) },
};

static int mi_new(ffui_loader *g, ffstr name, unsigned check)
{
	if (ffstr_eqcz(&name, "-")) {
		g->mi = ffui_menu_separator_new();
	} else {
		ffstr s = vars_val(&g->vars, name);
		char *sz = ffsz_dupstr(&s);
		if (check)
			g->mi = ffui_menu_check_new(sz);
		else
			g->mi = ffui_menu_new(sz);
		ffmem_free(sz);
	}

	add_ctx(g, mi_args);
	return 0;
}

static int mi_new_check(ffui_loader *g, ffstr name) { return mi_new(g, name, 1); }
static int mi_new_text(ffui_loader *g, ffstr name) { return mi_new(g, name, 0); }

// MENU
static const ffconf_arg menu_args[] = {
	{ "check_item",	T_OBJS_ARG,	_F(mi_new_check) },
	{ "item",		T_OBJS_ARG,	_F(mi_new_text) },
	{}
};

static int menu_new(ffui_loader *g, ffstr name)
{
	if (!(g->menu = g->getctl(g->udata, &name)))
		return FFUI_EINVAL;

	if (0 != ffui_menu_create(g->menu))
		return FFUI_ENOMEM;

	add_ctx(g, menu_args);
	return 0;
}

static int mmenu_new(ffui_loader *g, ffstr name)
{
	if (!(g->menu = ldr_getctl(g, name)))
		return FFUI_EINVAL;

	if (0 != ffui_menu_createmain(g->menu))
		return FFUI_ENOMEM;

	ffui_wnd_setmenu(g->wnd, g->menu);
	add_ctx(g, menu_args);
	return 0;
}


// LABEL
static int lbl_style(ffui_loader *g, ffstr val)
{
	if (ffstr_eqcz(&val, "horizontal"))
		g->f_horiz = 1;
	else
		return FFUI_EINVAL;
	return 0;
}
static int lbl_text(ffui_loader *g, ffstr val)
{
	val = vars_val(&g->vars, val);
	ffui_label_settextstr(g->lbl, &val);
	return 0;
}
static int lbl_done(ffui_loader *g)
{
	ctl_place(g->ctl, g);
	return 0;
}
static const ffconf_arg lbl_args[] = {
	{ "size",	T_INTLIST,	_F(ctl_size) }, // compat
	{ "style",	T_STRLIST,	_F(lbl_style) },
	{ "text",	T_STR,		_F(lbl_text) },
	{ NULL,		T_CLOSE,	_F(lbl_done) },
};

static int lbl_new(ffui_loader *g, ffstr name)
{
	if (!(g->ctl = ldr_getctl(g, name)))
		return FFUI_EINVAL;

	if (0 != ffui_label_create(g->lbl, g->wnd))
		return FFUI_ENOMEM;

	add_ctx(g, lbl_args);
	ctl_reset(g);
	return 0;
}


// IMAGE
static int img_done(ffui_loader *g)
{
	ctl_place(g->ctl, g);

	ffui_icon ico;
	if (icon_done(g, &ico))
		ffui_image_seticon(g->img, &ico);
	return 0;
}
static const ffconf_arg img_args[] = {
	{ "icon",	T_OBJ,		_F(icon_new) },
	{ NULL,		T_CLOSE,	_F(img_done) },
};
static int img_new(ffui_loader *g, ffstr name)
{
	if (!(g->ctl = ldr_getctl(g, name)))
		return FFUI_EINVAL;

	if (0 != ffui_image_create(g->img, g->wnd))
		return FFUI_ENOMEM;

	add_ctx(g, img_args);
	ctl_reset(g);
	return 0;
}


// BUTTON
static int btn_text(ffui_loader *g, ffstr val)
{
	val = vars_val(&g->vars, val);
	ffui_button_settextstr(g->btn, &val);
	return 0;
}
static int btn_style(ffui_loader *g, ffstr val)
{
	if (ffstr_eqcz(&val, "horizontal"))
		g->f_horiz = 1;
	else
		return FFUI_EINVAL;
	return 0;
}
static int btn_action(ffui_loader *g, ffstr val)
{
	if (!(g->btn->action_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}
static int btn_done(ffui_loader *g)
{
	ctl_place(g->ctl, g);

	ffui_icon ico;
	if (icon_done(g, &ico))
		ffui_button_seticon(g->btn, &ico);
	return 0;
}
static const ffconf_arg btn_args[] = {
	{ "action",	T_STR,		_F(btn_action) },
	{ "icon",	T_OBJ,		_F(icon_new) },
	{ "size",	T_INTLIST,	_F(ctl_size) }, // compat
	{ "style",	T_STRLIST,	_F(btn_style) },
	{ "text",	T_STR,		_F(btn_text) },
	{ "tooltip",T_STR,		_F(ctl_tooltip) }, // compat
	{ NULL,		T_CLOSE,	_F(btn_done) },
};

static int btn_new(ffui_loader *g, ffstr name)
{
	if (!(g->ctl = ldr_getctl(g, name)))
		return FFUI_EINVAL;

	if (0 != ffui_button_create(g->btn, g->wnd))
		return FFUI_ENOMEM;

	add_ctx(g, btn_args);
	ctl_reset(g);
	return 0;
}

// CHECKBOX
static int checkbox_text(ffui_loader *g, ffstr val)
{
	val = vars_val(&g->vars, val);
	ffui_checkbox_settextstr(g->cb, &val);
	return 0;
}
static int checkbox_action(ffui_loader *g, ffstr val)
{
	if (!(g->cb->action_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}
static const ffconf_arg checkbox_args[] = {
	{ "action",	T_STR,		_F(checkbox_action) },
	{ "style",	T_STRLIST,	_F(btn_style) },
	{ "text",	T_STR,		_F(checkbox_text) },
	{ NULL,		T_CLOSE,	_F(btn_done) },
};

static int checkbox_new(ffui_loader *g, ffstr name)
{
	if (!(g->ctl = ldr_getctl(g, name)))
		return FFUI_EINVAL;

	if (0 != ffui_checkbox_create(g->cb, g->wnd))
		return FFUI_ENOMEM;

	add_ctx(g, checkbox_args);
	ctl_reset(g);
	return 0;
}


// COMBOBOX
static int combobox_style(ffui_loader *g, ffstr val)
{
	if (ffstr_eqcz(&val, "editable"))
		g->f_cbx_editable = 1;
	else
		return btn_style(g, val);
	return 0;
}
static int combobox_on_change(ffui_loader *g, ffstr val)
{
	if (!(g->combo->change_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}
static int combobox_done(ffui_loader *g)
{
	if (g->f_cbx_editable)
		ffui_combo_create_editable(g->combo, g->wnd);
	else
		ffui_combo_create(g->combo, g->wnd);
	ctl_place(g->ctl, g);
	return 0;
}
static const ffconf_arg combobox_args[] = {
	{ "on_change",	T_STR,		_F(combobox_on_change) },
	{ "style",		T_STRLIST,	_F(combobox_style) },
	{ NULL,			T_CLOSE,	_F(combobox_done) },
};
static int combobox_new(ffui_loader *g, ffstr name)
{
	if (!(g->ctl = ldr_getctl(g, name)))
		return FFUI_EINVAL;
	add_ctx(g, combobox_args);
	ctl_reset(g);
	return 0;
}


// EDITBOX
static int edit_done(ffui_loader *g)
{
	ctl_place_f(g, g->ctl, CTL_PLACE_EXPAND | CTL_PLACE_FILL);
	return 0;
}
static int edit_text(ffui_loader *g, ffstr val)
{
	ffui_edit_settextstr(g->edit, &val);
	return 0;
}
static int edit_style(ffui_loader *g, ffstr val)
{
	if (ffstr_eqcz(&val, "password"))
		ffui_edit_password(g->edit, 1);
	else
		return btn_style(g, val);
	return 0;
}
static int edit_onchange(ffui_loader *g, ffstr val)
{
	if (!(g->edit->change_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}
static const ffconf_arg edit_args[] = {
	{ "onchange",T_STR,		_F(edit_onchange) },
	{ "style",	T_STRLIST,	_F(edit_style) },
	{ "text",	T_STR,		_F(edit_text) },
	{ NULL,		T_CLOSE,	_F(edit_done) },
};

static int edit_new(ffui_loader *g, ffstr name)
{
	if (!(g->ctl = ldr_getctl(g, name)))
		return FFUI_EINVAL;

	if (0 != ffui_edit_create(g->edit, g->wnd))
		return FFUI_ENOMEM;

	add_ctx(g, edit_args);
	ctl_reset(g);
	return 0;
}


// TEXT
static int text_done(ffui_loader *g)
{
	if (g->f_horiz) {
		ctl_place_f(g, g->ctl, CTL_PLACE_EXPAND | CTL_PLACE_FILL | CTL_PLACE_EXPAND_H);
	} else {
		void *scrl = gtk_scrolled_window_new(NULL, NULL);
		gtk_container_add(GTK_CONTAINER(scrl), g->text->h);
		gtk_scrolled_window_set_policy(scrl, GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
		g->hbox = NULL;
		gtk_box_pack_start(GTK_BOX(g->wnd->vbox), scrl, /*expand=*/1, /*fill=*/1, /*padding=*/0);
	}

	return 0;
}
static const ffconf_arg text_args[] = {
	{ "style",	T_STRLIST,	_F(btn_style) },
	{ NULL,		T_CLOSE,	_F(text_done) },
};

static int text_new(ffui_loader *g, ffstr name)
{
	if (!(g->ctl = ldr_getctl(g, name)))
		return FFUI_EINVAL;

	if (0 != ffui_text_create(g->text, g->wnd))
		return FFUI_ENOMEM;

	add_ctx(g, text_args);
	ctl_reset(g);
	return 0;
}


// TRACKBAR
static int trkbar_range(ffui_loader *g, ffint64 val)
{
	ffui_track_setrange(g->trkbar, val);
	return 0;
}
static int trkbar_style(ffui_loader *g, ffstr val)
{
	if (ffstr_eqcz(&val, "horizontal"))
		g->f_horiz = 1;
	else if (ffstr_eqcz(&val, "no_ticks")) {} // compat
	else if (ffstr_eqcz(&val, "both")) {} // compat
	else
		return FFUI_EINVAL;
	return 0;
}
static int trkbar_val(ffui_loader *g, ffint64 val)
{
	ffui_track_set(g->trkbar, val);
	return 0;
}
static int trkbar_onscroll(ffui_loader *g, ffstr val)
{
	if (!(g->trkbar->scroll_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}
static int trkbar_pagesize(ffui_loader *g, ffint64 val) { return 0; }
static int trkbar_done(ffui_loader *g)
{
	if (g->f_loadconf)
		return 0;
	ctl_place_f(g, g->ctl, CTL_PLACE_EXPAND | CTL_PLACE_FILL);
	return 0;
}
static const ffconf_arg trkbar_args[] = {
	{ "page_size",	T_INT32,	_F(trkbar_pagesize) }, // compat
	{ "range",		T_INT32,	_F(trkbar_range) },
	{ "scroll",		T_STR,		_F(trkbar_onscroll) },
	{ "scrolling",	T_STR,		_F(trkbar_onscroll) }, // compat
	{ "size",		T_INTLIST,	_F(ctl_size) }, // compat
	{ "style",		T_STRLIST,	_F(trkbar_style) },
	{ "value",		T_INT32,	_F(trkbar_val) },
	{ NULL,			T_CLOSE,	_F(trkbar_done) },
};

static int trkbar_new(ffui_loader *g, ffstr name)
{
	if (!(g->trkbar = ldr_getctl(g, name)))
		return FFUI_EINVAL;

	if (0 != ffui_track_create(g->trkbar, g->wnd))
		return FFUI_ENOMEM;
	add_ctx(g, trkbar_args);
	ctl_reset(g);
	return 0;
}


// TAB
static int tab_onchange(ffui_loader *g, ffstr val)
{
	if (!(g->tab->change_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}
static const ffconf_arg tab_args[] = {
	{ "onchange",	T_STR,	_F(tab_onchange) },
	{ "size",		T_INTLIST,	_F(ctl_size) }, // compat
	{}
};
static int tab_new(ffui_loader *g, ffstr name)
{
	if (!(g->tab = ldr_getctl(g, name)))
		return FFUI_EINVAL;

	if (0 != ffui_tab_create(g->tab, g->wnd))
		return FFUI_ENOMEM;
	add_ctx(g, tab_args);
	ctl_reset(g);
	return 0;
}


// VIEW COL
static int viewcol_width(ffui_loader *g, ffint64 val)
{
	ffui_viewcol_setwidth(&g->vicol, val);
	return 0;
}

static int viewcol_align(ffui_loader *g, ffstr val) { return 0; }

static int viewcol_done(ffui_loader *g)
{
	ffui_view_inscol(g->view, -1, &g->vicol);
	return 0;
}

static const ffconf_arg viewcol_args[] = {
	{ "align",	T_STR,		_F(viewcol_align) }, // compat
	{ "width",	T_INT32,	_F(viewcol_width) },
	{ NULL,		T_CLOSE,	_F(viewcol_done) },
};

static int viewcol_new(ffui_loader *g, ffstr name)
{
	ffui_viewcol_reset(&g->vicol);
	ffui_viewcol_setwidth(&g->vicol, 100);
	ffstr s = vars_val(&g->vars, name);
	ffui_viewcol_settext(&g->vicol, s.ptr, s.len);
	add_ctx(g, viewcol_args);
	return 0;
}

// VIEW

enum {
	VIEW_STYLE_EDITABLE,
	VIEW_STYLE_EXPLORER_THEME,
	VIEW_STYLE_GRID_LINES,
	VIEW_STYLE_MULTI_SELECT,
};
static const char view_styles_sorted[][16] = {
	"editable",
	"explorer_theme", // compat
	"grid_lines",
	"multi_select",
};
static int view_style(ffui_loader *g, ffstr val)
{
	int n = ffcharr_ifind_sorted(view_styles_sorted, FF_COUNT(view_styles_sorted), sizeof(view_styles_sorted[0]), val.ptr, val.len);
	switch (n) {

	case VIEW_STYLE_GRID_LINES:
		ffui_view_style(g->view, FFUI_VIEW_GRIDLINES, FFUI_VIEW_GRIDLINES);
		break;

	case VIEW_STYLE_MULTI_SELECT:
		ffui_view_style(g->view, FFUI_VIEW_MULTI_SELECT, FFUI_VIEW_MULTI_SELECT);
		break;

	case VIEW_STYLE_EDITABLE:
		ffui_view_style(g->view, FFUI_VIEW_EDITABLE, FFUI_VIEW_EDITABLE);
		break;

	case VIEW_STYLE_EXPLORER_THEME:
		break;

	default:
		return FFUI_EINVAL;
	}
	return 0;
}

static int view_double_click(ffui_loader *g, ffstr val)
{
	if (!(g->view->dblclick_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}

static int view_popup_menu(ffui_loader *g, ffstr val)
{
	ffui_menu *m = g->getctl(g->udata, &val);
	if (m == NULL)
		return FFUI_EINVAL;
	ffui_view_popupmenu(g->view, m);
	return 0;
}

static const ffconf_arg view_args[] = {
	{ "column",		T_OBJS_ARG,	_F(viewcol_new) },
	{ "double_click",T_STR,		_F(view_double_click) },
	{ "popup_menu",	T_STR,		_F(view_popup_menu) },
	{ "size",		T_INTLIST,	_F(ctl_size) }, // compat
	{ "style",		T_STRLIST,	_F(view_style) },
	{}
};

static int view_new(ffui_loader *g, ffstr name)
{
	if (!(g->ctl = ldr_getctl(g, name)))
		return FFUI_EINVAL;

	if (0 != ffui_view_create(g->view, g->wnd))
		return FFUI_ENOMEM;

	add_ctx(g, view_args);
	ctl_reset(g);
	return 0;
}


// STATUSBAR
static const ffconf_arg stbar_args[] = {
	{}
};
static int stbar_new(ffui_loader *g, ffstr name)
{
	if (!(g->ctl = ldr_getctl(g, name)))
		return FFUI_EINVAL;

	if (0 != ffui_status_create(g->ctl, g->wnd))
		return FFUI_ENOMEM;

	add_ctx(g, stbar_args);
	return 0;
}


// TRAYICON
static int tray_lclick(ffui_loader *g, ffstr val)
{
	if (!(g->trayicon->lclick_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}
static const ffconf_arg tray_args[] = {
	{ "lclick",	T_STR,	_F(tray_lclick) },
	{}
};
static int tray_new(ffui_loader *g, ffstr name)
{
	if (!(g->trayicon = ldr_getctl(g, name)))
		return FFUI_EINVAL;

	ffui_tray_create(g->trayicon, g->wnd);
	add_ctx(g, tray_args);
	return 0;
}


// DIALOG
static const ffconf_arg dlg_args[] = {
	{}
};
static int dlg_new(ffui_loader *g, ffstr name)
{
	if (!(g->dlg = g->getctl(g->udata, &name)))
		return FFUI_EINVAL;
	ffui_dlg_init(g->dlg);
	add_ctx(g, dlg_args);
	return 0;
}


// WINDOW
static int wnd_title(ffui_loader *g, ffstr val)
{
	val = vars_val(&g->vars, val);
	ffui_wnd_settextstr(g->wnd, &val);
	return 0;
}

static int wnd_popupfor(ffui_loader *g, ffstr val)
{
	void *parent;
	if (!(parent = g->getctl(g->udata, &val)))
		return FFUI_EINVAL;
	ffui_wnd_setpopup(g->wnd, parent);
	return 0;
}

static int wnd_on_close(ffui_loader *g, ffstr val)
{
	if (!(g->wnd->onclose_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}

static int wnd_position(ffui_loader *g, ffint64 v)
{
	int *i = &g->r.x;
	if (g->list_idx == 4)
		return FFUI_EINVAL;
	i[g->list_idx] = (int)v;
	if (g->list_idx == 3) {
		ffui_wnd_setplacement(g->wnd, 0, &g->r);
	}
	g->list_idx++;
	return 0;
}

static int wnd_icon(ffui_loader *g)
{
	ffmem_zero_obj(&g->ico);
	ffconf_scheme_addctx(g->cs, icon_args, &g->ico);
	return 0;
}

static int wnd_done(ffui_loader *g)
{
	if (g->ico.fn != NULL) {
		ffui_icon ico;
		char *sz = ffsz_allocfmt("%S/%s", &g->path, g->ico.fn);
		ffui_icon_load(&ico, sz);
		ffmem_free(sz);
		ffui_wnd_seticon(g->wnd, &ico);
		// ffmem_free(g->ico.fn);
		// ffmem_zero_obj(&g->ico);
	}

	if (g->accels.len != 0) {
		int r = ffui_wnd_hotkeys(g->wnd, (ffui_wnd_hotkey*)g->accels.ptr, g->accels.len);
		g->accels.len = 0;
		if (r != 0)
			return FFUI_ENOMEM;
	}

	g->wnd_complete = 1;
	return 0;
}

static const ffconf_arg wnd_args[] = {
	{ "button",		T_OBJS_ARG,	_F(btn_new) },
	{ "checkbox",	T_OBJS_ARG,	_F(checkbox_new) },
	{ "combobox",	T_OBJS_ARG,	_F(combobox_new) },
	{ "editbox",	T_OBJS_ARG,	_F(edit_new) },
	{ "icon",		T_OBJ,		_F(wnd_icon) },
	{ "image",		T_OBJS_ARG,	_F(img_new) },
	{ "label",		T_OBJS_ARG,	_F(lbl_new) },
	{ "listview",	T_OBJS_ARG,	_F(view_new) },
	{ "mainmenu",	T_OBJ_ARG,	_F(mmenu_new) },
	{ "on_close",	T_STR,		_F(wnd_on_close) },
	{ "popupfor",	T_STR,		_F(wnd_popupfor) },
	{ "position",	FFCONF_TINT32 | FFCONF_FSIGN | FFCONF_FLIST, _F(wnd_position) },
	{ "statusbar",	T_OBJ_ARG,	_F(stbar_new) },
	{ "tab",		T_OBJS_ARG,	_F(tab_new) },
	{ "text",		T_OBJS_ARG,	_F(text_new) },
	{ "title",		T_STR,		_F(wnd_title) },
	{ "trackbar",	T_OBJS_ARG,	_F(trkbar_new) },
	{ "trayicon",	T_OBJ_ARG,	_F(tray_new) },
	{ NULL,			T_CLOSE,	_F(wnd_done) },
};

static int wnd_new(ffui_loader *g, ffstr name)
{
	ffmem_free(g->wnd_name);  g->wnd_name = NULL;

	ffui_window *wnd;

	if (!(wnd = g->getctl(g->udata, &name)))
		return FFUI_EINVAL;
	ffmem_zero((ffbyte*)g + FF_OFF(ffui_loader, wnd), sizeof(ffui_loader) - FF_OFF(ffui_loader, wnd));
	g->wnd = wnd;
	if (!(g->wnd_name = ffsz_dupn(name.ptr, name.len)))
		return FFUI_ENOMEM;
	g->ctl = (ffui_ctl*)wnd;
	if (0 != ffui_wnd_create(wnd))
		return FFUI_ENOMEM;

	add_ctx(g, wnd_args);
	return 0;
}


// LANGUAGE
static int inc_lang_entry(ffui_loader *g, ffstr fn)
{
	ffstr lang = *ffconf_scheme_keyname(g->cs);
	unsigned def = 0;
	if (!((def = ffstr_eqz(&lang, "default"))
		|| ffstr_eq(&lang, g->language, 2)))
		return 0;
	if (g->lang_data.len != 0)
		return FFUI_EINVAL;

	if (ffstr_findanyz(&fn, "/\\") >= 0)
		return FFUI_EINVAL;

	int rc = FFUI_EINVAL;
	char *ffn = ffsz_allocfmt("%S/%S", &g->path, &fn);
	ffvec d = {};
	if (0 != fffile_readwhole(ffn, &d, 64*1024))
		goto end;

	rc = vars_load(&g->vars, *(ffstr*)&d);

end:
	if (rc == 0) {
		if (def)
			g->lang_data_def = d;
		else
			g->lang_data = d;
		ffvec_null(&d);
	}
	ffvec_free(&d);
	ffmem_free(ffn);
	return rc;
}
static const ffconf_arg inc_lang_args[] = {
	{ "*", T_STRMULTI, _F(inc_lang_entry) },
	{}
};
static int inc_lang(ffui_loader *g)
{
	add_ctx(g, inc_lang_args);
	return 0;
}


static const ffconf_arg top_args[] = {
	{ "dialog",	T_OBJS_ARG,	_F(dlg_new) },
	{ "include_language",	T_OBJ,	_F(inc_lang) },
	{ "menu",	T_OBJS_ARG,	_F(menu_new) },
	{ "window",	T_OBJS_ARG,	_F(wnd_new) },
	{}
};

void ffui_ldr_init(ffui_loader *g, ffui_ldr_getctl_t getctl, ffui_ldr_getcmd_t getcmd, void *udata)
{
	ffmem_zero_obj(g);
	g->getctl = getctl;
	g->getcmd = getcmd;
	g->udata = udata;
	vars_init(&g->vars);
}

void ffui_ldr_fin(ffui_loader *g)
{
	ffvec_free(&g->lang_data_def);
	ffvec_free(&g->lang_data);
	ffmem_free(g->ico_ctl.fn);
	ffmem_free(g->ico.fn);
	ffvec_free(&g->accels);
	ffmem_free(g->errstr);
	ffmem_free(g->wnd_name);
	vars_free(&g->vars);
}

int ffui_ldr_load(ffui_loader *g, const char *window)
{
	int r, r2 = 0;
	struct ffconf_obj c = {
		.lt.line = g->conf_line,
		.lt.line_off = g->conf_col,
	};

	ffconf_scheme cs = {};
	ffconf_scheme_addctx(&cs, top_args, g);
	g->cs = &cs;

	ffstr val = {};
	while (g->conf.len) {
		if (FFCONF_ERROR == (r = ffconf_obj_read(&c, &g->conf, &val)))
			goto end;
		_ffui_log("conf: %d: %S", ffconf_line(&c.lt), &val);

		if ((r2 = ffconf_scheme_process(&cs, r, val))) {
			r = r2;
			goto end;
		}

		if (g->wnd_complete && window && ffsz_eq(g->wnd_name, window)) {
			// Stop after we've finished loading the target window
			g->conf_line = c.lt.line;
			g->conf_col = c.lt.line_off;
			break;
		}
	}

	ffstr_null(&val);
	r = 0;

end:
	ffconf_scheme_destroy(&cs);
	if (ffconf_obj_fin(&c) && r == 0)
		r = FFCONF_ERROR;

	if (r != 0) {
		char errbuf[100];
		const char *err = ffconf_error(&c.lt);
		if (r2 != 0) {
			err = cs.errmsg;
			if (r2 != FFCONF_ERROR) {
				ffsz_format(errbuf, sizeof(errbuf), "%d", r2);
				err = errbuf;
			}
		}
		ffmem_free(g->errstr);
		g->errstr = ffsz_allocfmt("%u:%u: near \"%S\": %s"
			, (int)ffconf_line(&c.lt), (int)ffconf_col(&c.lt)
			, &val
			, err);
	}

	return r;
}

int ffui_ldr_loadfile(ffui_loader *g, const char *fn)
{
	int r = -1;
	ffvec data = {};
	if (fffile_readwhole(fn, &data, 1*1024*1024)) {
		ffmem_free(g->errstr);
		g->errstr = ffsz_allocfmt("%s: %s", fn, fferr_strptr(fferr_last()));
		goto end;
	}

	ffpath_splitpath(fn, ffsz_len(fn), &g->path, NULL);
	g->conf = *(ffstr*)&data;
	r = ffui_ldr_load(g, NULL);

end:
	ffvec_free(&data);
	return r;
}

void ffui_ldr_loadconf(ffui_loader *g, ffstr data)
{
	struct ffconf_obj conf = {};
	ffconf_scheme cs = {};
	ffstr line, ctx;
	g->cs = &cs;

	while (data.len != 0) {
		ffstr_splitby(&data, '\n', &line, &data);
		ffstr_trimwhite(&line);
		if (line.len == 0)
			continue;

		// "ctx0[.ctx1].key val" -> [ "ctx0[.ctx1]", "key val" ]
		ffssize spc, dot;
		if ((spc = ffstr_findanyz(&line, " \t")) < 0)
			continue;
		if ((dot = ffs_rfindchar(line.ptr, spc, '.')) < 0)
			continue;
		ffstr_set(&ctx, line.ptr, dot);
		ffstr_shift(&line, dot + 1);
		if (ctx.len == 0 || line.len == 0)
			continue;

		if (!(g->ctl = g->getctl(g->udata, &ctx)))
			continue; // couldn't find the control by path "ctx0[.ctx1]"

		switch (g->ctl->uid) {
		case FFUI_UID_WINDOW:
			g->wnd = (void*)g->ctl;
			ffconf_scheme_addctx(&cs, wnd_args, g);  break;

		case FFUI_UID_TRACKBAR:
			ffconf_scheme_addctx(&cs, trkbar_args, g);  break;

		default:
			continue;
		}

		ffbool lf = 0;
		for (;;) {
			ffstr val;
			int r = ffconf_obj_read(&conf, &line, &val);
			if (r == FFCONF_ERROR) {
				goto end;
			} else if (r == FFCONF_MORE && !lf) {
				// on the next iteration the parser will validate the input line
				ffstr_setcz(&line, "\n");
				lf = 1;
				continue;
			}

			r = ffconf_scheme_process(&cs, r, val);
			if (r != 0)
				goto end;
			if (lf)
				break;
		}

		ffconf_scheme_destroy(&cs);
		ffmem_zero_obj(&cs);
		ffconf_obj_fin(&conf);
		ffmem_zero_obj(&conf);
	}

end:
	ffconf_scheme_destroy(&cs);
	ffconf_obj_fin(&conf);
}

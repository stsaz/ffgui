/** GUI-winapi loader
2014, Simon Zolin */

#include <ffgui/winapi/loader.h>
#include <ffsys/path.h>
#include <ffsys/file.h>
#include <ffsys/dylib.h>


static void* ldr_getctl(ffui_loader *g, const ffstr *name);

enum {
	FFUI_EINVAL = 100,
	FFUI_ENOMEM,
	FFUI_ESYS,
};

#define _F(f)  (ffsize)(f)
#define T_CLOSE  FFCONF_TCLOSE
#define T_OBJ  FFCONF_TOBJ
#define T_OBJMULTI  (FFCONF_TOBJ | FFCONF_FMULTI)
#define T_INT32  FFCONF_TINT32
#define T_INTLIST  (FFCONF_TINT32 | FFCONF_FLIST)
#define T_INTLIST_S  (FFCONF_TINT32 | FFCONF_FSIGN | FFCONF_FLIST)
#define T_STR  FFCONF_TSTR
#define T_STRMULTI  (FFCONF_TSTR | FFCONF_FMULTI)
#define T_STRLIST  (FFCONF_TSTR | FFCONF_FLIST)
#define FF_XSPACE  2
#define FF_YSPACE  2

static void state_reset(ffui_loader *g)
{
	g->list_idx = 0;
	g->style_reset = 0;
}
static void state_reset2(ffui_loader *g)
{
	g->r.x = 0;
	g->r.y = 0;
	g->r.cx = 200;
	g->r.cy = 20;
	g->style_horizontal = 0;
	g->auto_pos = 1;
	g->man_pos = 0;
	state_reset(g);
}

static void ctl_create_prepare(ffui_loader *g)
{
	// Default font
	if (g->wnd->font == NULL) {
		ffui_font font = {};
		ffstr font_name = FFSTR_Z("Arial");
		ffui_font_set(&font, &font_name, 10, 0);
		g->wnd->font = ffui_font_create(&font);
	}
}

static void ctl_setpos(ffui_loader *g)
{
	ffui_pos *p = &g->prev_ctl_pos;
	if (g->auto_pos) {
		g->auto_pos = 0;
		g->man_pos = 1;

		if (g->style_horiz_prev) {
			// place this control to the right of the previous control
			g->r.x = p->x + p->cx + FF_XSPACE;
			g->r.y = p->y;

			p->x = g->r.x;
			p->cx = g->r.cx;
			p->y = g->r.y;
			p->cy = ffmax(p->cy, g->r.cy);

		} else {
			// place this control below the previous control line
			g->r.x = 0;
			g->r.y = p->y + p->cy + FF_YSPACE;

			p->x = 0;
			p->cx = g->r.cx;
			p->y = g->r.y;
			p->cy = g->r.cy;
		}

		g->style_horiz_prev = g->style_horizontal;
		g->style_horizontal = 0;

	} else {
		p->x = g->r.x;
		p->cx = g->r.cx;
		p->y = g->r.y;
		p->cy = ffmax(p->cy, g->r.cy);
	}

	if (g->man_pos) {
		g->man_pos = 0;
		ffui_setposrect(g->actl.ctl, &g->r, 0);
		g->edge_right = ffmax(g->edge_right, (uint)g->r.x + g->r.cx);
		g->edge_bottom = ffmax(g->edge_bottom, (uint)g->r.y + g->r.cy);
	}
}
static int ctl_pos(ffconf_scheme *cs, ffui_loader *g, ffint64 val)
{
	if (g->list_idx == 4)
		return FFUI_EINVAL;
	int *i = &g->r.x;
	i[g->list_idx] = (int)val;
	g->man_pos = 1;
	g->auto_pos = 0;
	g->list_idx++;
	return 0;
}
static int ctl_size(ffconf_scheme *cs, ffui_loader *g, ffint64 val)
{
	if (g->list_idx == 2)
		return FFUI_EINVAL;
	if (g->list_idx == 0)
		g->r.cx = (int)val;
	else
		g->r.cy = (int)val;
	g->auto_pos = 1;
	g->list_idx++;
	return 0;
}

#define F_RESIZE_CX 1
#define F_RESIZE_CY 2

static int ctl_resize(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (ffstr_eqcz(&val, "cx"))
		g->resize_flags |= F_RESIZE_CX;
	else if (ffstr_eqcz(&val, "cy"))
		g->resize_flags |= F_RESIZE_CY;
	return 0;
}
static void ctl_autoresize(ffui_loader *g)
{
	if (g->resize_flags == 0)
		return;
	ffui_paned *pn = ffmem_new(ffui_paned);
	*ffvec_pushT(&g->paned_array, ffui_paned*) = pn;
	pn->items[0].it = g->actl.ctl;
	pn->items[0].cx = !!(g->resize_flags & F_RESIZE_CX);
	pn->items[0].cy = !!(g->resize_flags & F_RESIZE_CY);
	ffui_paned_create(pn, g->wnd);
	g->resize_flags = 0;
}
static int ctl_style(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (ffstr_eqcz(&val, "visible"))
		;
	else if (ffstr_eqcz(&val, "horizontal"))
		g->style_horizontal = 1;
	else
		return FFUI_EINVAL;

	return 0;
}
static int ctl_done(ffconf_scheme *cs, ffui_loader *g)
{
	ctl_setpos(g);
	ctl_autoresize(g);
	ffui_show(g->actl.ctl, 1);
	return 0;
}

static int ico_size(ffconf_scheme *cs, _ffui_ldr_icon_t *ico, ffint64 val)
{
	switch (ico->ldr->list_idx) {
	case 0:
		ico->cx = val; break;
	case 1:
		ico->cy = val; break;
	default:
		return FFUI_EINVAL;
	}
	ico->ldr->list_idx++;
	return 0;
}
static int ico_done(ffconf_scheme *cs, _ffui_ldr_icon_t *ico)
{
	char *p, fn[4096];

	if (ico->resource != 0) {
		wchar_t wname[256];
		ffs_format(fn, sizeof(fn), "#%u%Z", ico->resource);
		size_t wname_len = FF_COUNT(wname);
		ffs_utow(wname, &wname_len, fn, -1);
		if (0 != ffui_icon_load_res(&ico->icon, ico->ldr->hmod_resource, wname, ico->cx, ico->cy)) {
			if (ico->cx == 0)
				return FFUI_ESYS;
			if (0 != ffui_icon_load_res(&ico->icon, ico->ldr->hmod_resource, wname, 0, 0))
				return FFUI_ESYS;
		}
		if (ico->load_small
			&& 0 != ffui_icon_load_res(&ico->icon_small, ico->ldr->hmod_resource, wname, 16, 16)) {

			if (0 != ffui_icon_load_res(&ico->icon_small, ico->ldr->hmod_resource, wname, 0, 0))
				return FFUI_ESYS;
		}
		return 0;
	}

	p = fn + ffmem_ncopy(fn, FF_COUNT(fn), ico->ldr->path.ptr, ico->ldr->path.len);
	ffsz_copyn(p, fn + FF_COUNT(fn) - p, ico->fn.ptr, ico->fn.len);
	ffstr_free(&ico->fn);
	if (ico->cx != 0) {
		if (0 != ffui_icon_loadimg(&ico->icon, fn, ico->cx, ico->cy, FFUI_ICON_DPISCALE)) {
			//Note: winXP can't read PNG-compressed icons.  Load the first icon.
			if (0 != ffui_icon_load(&ico->icon, fn, 0, 0))
				return FFUI_ESYS;
		}
	} else {
		if (0 != ffui_icon_load(&ico->icon, fn, 0, 0))
			return FFUI_ESYS;
		if (ico->load_small && 0 != ffui_icon_load(&ico->icon_small, fn, 0, FFUI_ICON_SMALL))
			return FFUI_ESYS;
	}
	return 0;
}
static const ffconf_arg icon_args[] = {
	{ "filename",	T_STR,		FF_OFF(_ffui_ldr_icon_t, fn) },
	{ "index",		T_INT32,	FF_OFF(_ffui_ldr_icon_t, idx) },
	{ "resource",	T_INT32,	FF_OFF(_ffui_ldr_icon_t, resource) },
	{ "size",		T_INTLIST,	_F(ico_size) },
	{ NULL,			T_CLOSE,	_F(ico_done) },
};


static int mi_submenu(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	ffui_menu *sub;

	if (NULL == (sub = g->getctl(g->udata, &val)))
		return FFUI_EINVAL;

	ffui_menu_setsubmenu(&g->menuitem.mi, sub->h);
	return 0;
}

static int mi_style(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (ffstr_eqcz(&val, "checked"))
		ffui_menu_addstate(&g->menuitem.mi, FFUI_MENU_CHECKED);

	else if (ffstr_eqcz(&val, "default"))
		ffui_menu_addstate(&g->menuitem.mi, FFUI_MENU_DEFAULT);

	else if (ffstr_eqcz(&val, "disabled"))
		ffui_menu_addstate(&g->menuitem.mi, FFUI_MENU_DISABLED);

	else if (ffstr_eqcz(&val, "radio"))
		ffui_menu_settype(&g->menuitem.mi, FFUI_MENU_RADIOCHECK);

	else
		return FFUI_EINVAL;

	return 0;
}

static int mi_action(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	int id;
	if (0 == (id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	ffui_menu_setcmd(&g->menuitem.mi, id);
	return 0;
}

/** Append hotkey to menu item text */
static void ffui_menu_sethotkey(ffui_menuitem *mi, const char *s, size_t len)
{
	wchar_t *w = mi->dwTypeData;
	size_t textlen = wcslen(w);
	size_t cap = textlen + FFS_LEN("\t") + len + 1;
	if (mi->dwTypeData == NULL)
		return;

	if (NULL == (w = ffmem_realloc(w, cap * sizeof(wchar_t))))
		return;
	mi->dwTypeData = w;
	w += textlen;
	*w++ = '\t';
	ffsz_utow_n(w, cap, s, len);
}

static int mi_hotkey(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	ffui_wnd_hotkey *a;
	uint hk;

	if (NULL == (a = ffvec_pushT(&g->accels, ffui_wnd_hotkey)))
		return FFUI_ENOMEM;

	if (0 == (hk = ffui_hotkey_parse(val.ptr, val.len)))
		return FFUI_EINVAL;

	a->hk = hk;

	ffui_menu_sethotkey(&g->menuitem.mi, val.ptr, val.len);
	g->menuitem.iaccel = 1;
	return 0;
}

static int mi_done(ffconf_scheme *cs, ffui_loader *g)
{
	if (g->menuitem.iaccel && g->menuitem.mi.wID != 0) {
		ffui_wnd_hotkey *hk = ffslice_lastT(&g->accels, ffui_wnd_hotkey);
		hk->cmd = g->menuitem.mi.wID;
	}

	if (0 != ffui_menu_append(g->menu, &g->menuitem.mi))
		return FFUI_ENOMEM;
	return 0;
}
static const ffconf_arg menuitem_args[] = {
	{ "action",		T_STR,		_F(mi_action) },
	{ "hotkey",		T_STR,		_F(mi_hotkey) },
	{ "style",		T_STRLIST,	_F(mi_style) },
	{ "submenu",	T_STR,		_F(mi_submenu) },
	{ NULL,			T_CLOSE,	_F(mi_done) },
};
static int new_menuitem(ffconf_scheme *cs, ffui_loader *g)
{
	ffmem_zero_obj(&g->menuitem);

	if (ffstr_eqcz(&cs->objval, "-"))
		ffui_menu_settype(&g->menuitem.mi, FFUI_MENU_SEPARATOR);
	else {
		ffstr s = vars_val(&g->vars, cs->objval);
		ffui_menu_settextstr(&g->menuitem.mi, &s);
	}

	state_reset(g);
	ffconf_scheme_addctx(cs, menuitem_args, g);
	return 0;
}

static const ffconf_arg menu_args[] = {
	{ "check_item",	T_OBJMULTI,	_F(new_menuitem) },
	{ "item",		T_OBJMULTI,	_F(new_menuitem) },
	{}
};
static int new_menu(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->menu = g->getctl(g->udata, &cs->objval)))
		return FFUI_EINVAL;

	if (0 != ffui_menu_create(g->menu))
		return FFUI_ENOMEM;
	state_reset(g);
	ffconf_scheme_addctx(cs, menu_args, g);
	return 0;
}

static int new_mmenu(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->menu = ldr_getctl(g, &cs->objval)))
		return FFUI_EINVAL;

	ffui_menu_createmain(g->menu);

	if (!SetMenu(g->wnd->h, g->menu->h))
		return FFUI_ENOMEM;

	state_reset(g);
	ffconf_scheme_addctx(cs, menu_args, g);
	return 0;
}


static int trkbar_style(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (ffstr_eqcz(&val, "no_ticks"))
		_ffui_style_set(g->actl.trkbar, TBS_NOTICKS);
	else if (ffstr_eqcz(&val, "both"))
		_ffui_style_set(g->actl.trkbar, TBS_BOTH);
	else if (ffstr_eqcz(&val, "horizontal"))
		g->style_horizontal = 1;
	return 0;
}
static int trkbar_range(ffconf_scheme *cs, ffui_loader *g, ffint64 val)
{
	ffui_track_setrange(g->actl.trkbar, val);
	return 0;
}
static int trkbar_val(ffconf_scheme *cs, ffui_loader *g, ffint64 val)
{
	ffui_track_set(g->actl.trkbar, val);
	return 0;
}
static int trkbar_pagesize(ffconf_scheme *cs, ffui_loader *g, ffint64 val)
{
	ffui_track_setpage(g->actl.trkbar, val);
	return 0;
}
static int trkbar_onscroll(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (0 == (g->actl.trkbar->scroll_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}
static int trkbar_onscrolling(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (0 == (g->actl.trkbar->scrolling_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}
static const ffconf_arg trkbar_args[] = {
	{ "onscroll",	T_STR,		_F(trkbar_onscroll) },
	{ "onscrolling",T_STR,		_F(trkbar_onscrolling) },
	{ "page_size",	T_INT32,	_F(trkbar_pagesize) },
	{ "position",	T_INTLIST_S,_F(ctl_pos) },
	{ "range",		T_INT32,	_F(trkbar_range) },
	{ "resize",		T_STRLIST,	_F(ctl_resize) },
	{ "size",		T_INTLIST,	_F(ctl_size) },
	{ "style",		T_STRLIST,	_F(trkbar_style) },
	{ "value",		T_INT32,	_F(trkbar_val) },
	{ NULL,			T_CLOSE,	_F(ctl_done) },
};
static int new_trkbar(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->actl.trkbar = ldr_getctl(g, &cs->objval)))
		return FFUI_EINVAL;

	if (0 != ffui_track_create(g->actl.trkbar, g->wnd))
		return FFUI_ENOMEM;
	state_reset2(g);
	ffconf_scheme_addctx(cs, trkbar_args, g);
	return 0;
}


static const ffconf_arg pgsbar_args[] = {
	{ "position",	T_INTLIST_S,_F(ctl_pos) },
	{ "style",		T_STRLIST,	_F(ctl_style) },
	{ NULL,			T_CLOSE,	_F(ctl_done) },
};
static int new_pgsbar(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->actl.ctl = ldr_getctl(g, &cs->objval)))
		return FFUI_EINVAL;

	if (0 != ffui_progress_create(g->actl.ctl, g->wnd))
		return FFUI_ENOMEM;
	state_reset(g);
	ffconf_scheme_addctx(cs, pgsbar_args, g);
	return 0;
}


static int stbar_style(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	return 0;
}
static int stbar_parts(ffconf_scheme *cs, ffui_loader *g, ffint64 val)
{
	int *it = ffvec_pushT(&g->sb_parts, int);
	if (it == NULL)
		return FFUI_ENOMEM;
	*it = (int)val;
	return 0;
}
static int stbar_done(ffconf_scheme *cs, ffui_loader *g)
{
	ffui_status_setparts(g->actl.ctl, g->sb_parts.len, g->sb_parts.ptr);
	ffvec_free(&g->sb_parts);
	ffui_show(g->actl.ctl, 1);
	return 0;
}
static const ffconf_arg stbar_args[] = {
	{ "parts",		T_INTLIST_S,_F(stbar_parts) },
	{ "style",		T_STRLIST,	_F(stbar_style) },
	{ NULL,			T_CLOSE,	_F(stbar_done) },
};
static int new_stbar(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->actl.ctl = ldr_getctl(g, &cs->objval)))
		return FFUI_EINVAL;

	if (0 != ffui_status_create(g->actl.stbar, g->wnd))
		return FFUI_ENOMEM;
	ffvec_null(&g->sb_parts);
	state_reset(g);
	ffconf_scheme_addctx(cs, stbar_args, g);
	return 0;
}


static int tray_style(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (ffstr_eqcz(&val, "visible"))
		g->tr.show = 1;
	else
		return FFUI_EINVAL;
	return 0;
}
static int tray_pmenu(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	ffui_menu *m = g->getctl(g->udata, &val);
	if (m == NULL)
		return FFUI_EINVAL;
	g->wnd->trayicon->pmenu = m;
	return 0;
}
static int tray_icon(ffconf_scheme *cs, ffui_loader *g)
{
	ffmem_zero_obj(&g->ico_ctl);
	g->ico_ctl.ldr = g;
	g->ico_ctl.cx = g->ico_ctl.cy = 16;
	state_reset(g);
	ffconf_scheme_addctx(cs, icon_args, &g->ico_ctl);
	return 0;
}
static int tray_lclick(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (0 == (g->wnd->trayicon->lclick_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}
static int tray_done(ffconf_scheme *cs, ffui_loader *g)
{
	if (g->ico_ctl.icon.h != NULL)
		ffui_tray_seticon(g->tray, &g->ico_ctl.icon);

	if (g->tr.show && 0 != ffui_tray_show(g->tray, 1))
		return FFUI_ENOMEM;

	return 0;
}
static const ffconf_arg tray_args[] = {
	{ "icon",		T_OBJ,		_F(tray_icon) },
	{ "lclick",		T_STR,		_F(tray_lclick) },
	{ "popup_menu",	T_STR,		_F(tray_pmenu) },
	{ "style",		T_STRLIST,	_F(tray_style) },
	{ NULL,			T_CLOSE,	_F(tray_done) },
};
static int new_tray(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->tray = ldr_getctl(g, &cs->objval)))
		return FFUI_EINVAL;

	ffui_tray_create(g->tray, g->wnd);
	ffmem_zero_obj(&g->ico_ctl);
	state_reset(g);
	ffconf_scheme_addctx(cs, tray_args, g);
	return 0;
}


static int font_name(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	ffui_font_set(&g->fnt, &val, 0, 0);
	return 0;
}

static int font_height(ffconf_scheme *cs, ffui_loader *g, ffint64 val)
{
	ffui_font_set(&g->fnt, NULL, (int)val, 0);
	return 0;
}

static int font_style(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	uint f = 0;
	if (ffstr_eqcz(&val, "bold"))
		f = FFUI_FONT_BOLD;
	else if (ffstr_eqcz(&val, "italic"))
		f = FFUI_FONT_ITALIC;
	else if (ffstr_eqcz(&val, "underline"))
		f = FFUI_FONT_UNDERLINE;
	else
		return FFUI_EINVAL;
	ffui_font_set(&g->fnt, NULL, 0, f);
	return 0;
}

static int font_done(ffconf_scheme *cs, ffui_loader *g)
{
	HFONT f;
	f = ffui_font_create(&g->fnt);
	if (f == NULL)
		return FFUI_ENOMEM;
	if (g->actl.ctl == (void*)g->wnd)
		g->wnd->font = f;
	else {
		ffui_ctl_send(g->actl.ctl, WM_SETFONT, f, 0);
		g->actl.ctl->font = f;
	}
	return 0;
}
static const ffconf_arg font_args[] = {
	{ "height",		T_INT32,	_F(font_height) },
	{ "name",		T_STR,		_F(font_name) },
	{ "style",		T_STRLIST,	_F(font_style) },
	{ NULL,			T_CLOSE,	_F(font_done) },
};


static int label_text(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	val = vars_val(&g->vars, val);
	ffui_settextstr(g->actl.ctl, &val);
	return 0;
}

static int label_font(ffconf_scheme *cs, ffui_loader *g)
{
	ffmem_zero(&g->fnt, sizeof(LOGFONT));
	g->fnt.lf.lfHeight = 15;
	g->fnt.lf.lfWeight = FW_NORMAL;
	state_reset(g);
	ffconf_scheme_addctx(cs, font_args, g);
	return 0;
}

static const char *const _ffpic_clrstr[] = {
	"aqua",
	"black",
	"blue",
	"fuchsia",
	"green",
	"grey",
	"lime",
	"maroon",
	"navy",
	"olive",
	"orange",
	"purple",
	"red",
	"silver",
	"teal",
	"white",
	"yellow",
};
static const uint ffpic_clr_a[] = {
	/*aqua*/	0x7fdbff,
	/*black*/	0x111111,
	/*blue*/	0x0074d9,
	/*fuchsia*/	0xf012be,
	/*green*/	0x2ecc40,
	/*grey*/	0xaaaaaa,
	/*lime*/	0x01ff70,
	/*maroon*/	0x85144b,
	/*navy*/	0x001f3f,
	/*olive*/	0x3d9970,
	/*orange*/	0xff851b,
	/*purple*/	0xb10dc9,
	/*red*/		0xff4136,
	/*silver*/	0xdddddd,
	/*teal*/	0x39cccc,
	/*white*/	0xffffff,
	/*yellow*/	0xffdc00,
};

uint ffpic_color3(const char *s, ffsize len, const uint *clrs)
{
	ffssize n;
	uint clr = (uint)-1;

	if (len == FFS_LEN("#rrggbb") && s[0] == '#') {
		if (FFS_LEN("rrggbb") != ffs_toint(s + 1, len - 1, &clr, FFS_INT32 | FFS_INTHEX))
			goto err;

	} else {
		if (-1 == (n = ffszarr_ifindsorted(_ffpic_clrstr, FF_COUNT(_ffpic_clrstr), s, len)))
			goto err;
		clr = clrs[n];
	}

	//LE: BGR0 -> RGB0
	//BE: 0RGB -> RGB0
	union {
		uint i;
		byte b[4];
	} u;
	u.b[0] = ((clr & 0xff0000) >> 16);
	u.b[1] = ((clr & 0x00ff00) >> 8);
	u.b[2] = (clr & 0x0000ff);
	u.b[3] = 0;
	clr = u.i;

err:
	return clr;
}

static int label_color(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	uint clr;

	if ((uint)-1 == (clr = ffpic_color3(val.ptr, val.len, ffpic_clr_a)))
		return FFUI_EINVAL;

	g->actl.lbl->color = clr;
	return 0;
}

static int label_cursor(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (ffstr_ieqcz(&val, "hand"))
		ffui_label_setcursor(g->actl.lbl, FFUI_CUR_HAND);
	else
		return FFUI_EINVAL;
	return 0;
}

static int label_action(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (0 == (g->actl.lbl->click_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}
static const ffconf_arg label_args[] = {
	{ "color",		T_STR,		_F(label_color) },
	{ "cursor",		T_STR,		_F(label_cursor) },
	{ "font",		T_OBJ,		_F(label_font) },
	{ "onclick",	T_STR,		_F(label_action) },
	{ "position",	T_INTLIST_S,_F(ctl_pos) },
	{ "resize",		T_STRLIST,	_F(ctl_resize) },
	{ "size",		T_INTLIST,	_F(ctl_size) },
	{ "style",		T_STRLIST,	_F(ctl_style) },
	{ "text",		T_STR,		_F(label_text) },
	{ NULL,			T_CLOSE,	_F(ctl_done) },
};
static int new_label(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->actl.ctl = ldr_getctl(g, &cs->objval)))
		return FFUI_EINVAL;

	ctl_create_prepare(g);

	if (0 != ffui_label_create(g->actl.lbl, g->wnd))
		return FFUI_ENOMEM;

	state_reset2(g);
	g->r.cx = 400;
	ffconf_scheme_addctx(cs, label_args, g);
	return 0;
}


static int image_icon(ffconf_scheme *cs, ffui_loader *g)
{
	ffmem_zero_obj(&g->ico_ctl);
	g->ico_ctl.ldr = g;
	state_reset(g);
	ffconf_scheme_addctx(cs, icon_args, &g->ico_ctl);
	return 0;
}
static int image_action(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (0 == (g->actl.img->click_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}
static int image_done(ffconf_scheme *cs, ffui_loader *g)
{
	if (g->ico_ctl.icon.h != NULL)
		ffui_img_set(g->actl.img, &g->ico_ctl.icon);
	ctl_setpos(g);
	ffui_show(g->actl.ctl, 1);
	return 0;
}
static const ffconf_arg image_args[] = {
	{ "icon",		T_OBJ,		_F(image_icon) },
	{ "onclick",	T_STR,		_F(image_action) },
	{ "position",	T_INTLIST_S,_F(ctl_pos) },
	{ "size",		T_INTLIST,	_F(ctl_size) },
	{ "style",		T_STRLIST,	_F(ctl_style) },
	{ NULL,			T_CLOSE,	_F(image_done) },
};
static int new_image(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->actl.ctl = ldr_getctl(g, &cs->objval)))
		return FFUI_EINVAL;

	if (0 != ffui_img_create(g->actl.img, g->wnd))
		return FFUI_ENOMEM;

	state_reset2(g);
	ffconf_scheme_addctx(cs, image_args, g);
	return 0;
}


static int ctl_tooltip(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	ffui_wnd_tooltip(g->wnd, g->actl.ctl, val.ptr, val.len);
	return 0;
}

static int btn_action(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (0 == (g->actl.btn->action_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}
static int btn_done(ffconf_scheme *cs, ffui_loader *g)
{
	if (g->ico_ctl.icon.h != NULL)
		ffui_button_seticon(g->actl.btn, &g->ico_ctl.icon);

	ctl_done(cs, g);
	return 0;
}
static const ffconf_arg btn_args[] = {
	{ "action",		T_STR,		_F(btn_action) },
	{ "font",		T_OBJ,		_F(label_font) },
	{ "icon",		T_OBJ,		_F(image_icon) },
	{ "position",	T_INTLIST_S,_F(ctl_pos) },
	{ "resize",		T_STRLIST,	_F(ctl_resize) },
	{ "size",		T_INTLIST,	_F(ctl_size) },
	{ "style",		T_STRLIST,	_F(ctl_style) },
	{ "text",		T_STR,		_F(label_text) },
	{ "tooltip",	T_STR,		_F(ctl_tooltip) },
	{ NULL,			T_CLOSE,	_F(btn_done) },
};
static int new_button(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->actl.ctl = ldr_getctl(g, &cs->objval)))
		return FFUI_EINVAL;

	ctl_create_prepare(g);

	if (0 != ffui_button_create(g->actl.ctl, g->wnd))
		return FFUI_ENOMEM;

	ffmem_zero_obj(&g->ico_ctl);
	state_reset2(g);
	ffconf_scheme_addctx(cs, btn_args, g);
	return 0;
}


static int chbox_style(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (ffstr_eqcz(&val, "checked"))
		ffui_checkbox_check(g->actl.btn, 1);
	else
		return ctl_style(cs, g, val);
	return 0;
}
static const ffconf_arg chbox_args[] = {
	{ "action",		T_STR,		_F(btn_action) },
	{ "font",		T_OBJ,		_F(label_font) },
	{ "position",	T_INTLIST_S,_F(ctl_pos) },
	{ "resize",		T_STRLIST,	_F(ctl_resize) },
	{ "size",		T_INTLIST,	_F(ctl_size) },
	{ "style",		T_STRLIST,	_F(chbox_style) },
	{ "text",		T_STR,		_F(label_text) },
	{ "tooltip",	T_STR,		_F(ctl_tooltip) },
	{ NULL,			T_CLOSE,	_F(ctl_done) },
};
static int new_checkbox(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->actl.ctl = ldr_getctl(g, &cs->objval)))
		return FFUI_EINVAL;

	ctl_create_prepare(g);

	if (0 != ffui_checkbox_create(g->actl.ctl, g->wnd))
		return FFUI_ENOMEM;

	state_reset2(g);
	g->r.cx = 400;
	ffconf_scheme_addctx(cs, chbox_args, g);
	return 0;
}


static int edit_style(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (ffstr_eqcz(&val, "password"))
		ffui_edit_password(g->actl.ctl, 1);

	else if (ffstr_eqcz(&val, "readonly"))
		ffui_edit_readonly(g->actl.ctl, 1);

	else
		return ctl_style(cs, g, val);

	return 0;
}
static int edit_action(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (0 == (g->actl.edit->change_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}
static const ffconf_arg editbox_args[] = {
	{ "font",		T_OBJ,		_F(label_font) },
	{ "onchange",	T_STR,		_F(edit_action) },
	{ "position",	T_INTLIST_S,_F(ctl_pos) },
	{ "resize",		T_STRLIST,	_F(ctl_resize) },
	{ "size",		T_INTLIST,	_F(ctl_size) },
	{ "style",		T_STRLIST,	_F(edit_style) },
	{ "text",		T_STR,		_F(label_text) },
	{ "tooltip",	T_STR,		_F(ctl_tooltip) },
	{ NULL,			T_CLOSE,	_F(ctl_done) },
};
static int new_editbox(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->actl.ctl = ldr_getctl(g, &cs->objval)))
		return FFUI_EINVAL;

	ctl_create_prepare(g);

	int r;
	if (ffsz_eq(cs->arg->name, "text"))
		r = ffui_text_create(g->actl.ctl, g->wnd);
	else
		r = ffui_edit_create(g->actl.ctl, g->wnd);
	if (r != 0)
		return FFUI_ENOMEM;

	state_reset2(g);
	g->resize_flags |= F_RESIZE_CX;
	ffconf_scheme_addctx(cs, editbox_args, g);
	return 0;
}


static const ffconf_arg combx_args[] = {
	{ "font",		T_OBJ,		_F(label_font) },
	{ "position",	T_INTLIST_S,_F(ctl_pos) },
	{ "resize",		T_STRLIST,	_F(ctl_resize) },
	{ "size",		T_INTLIST,	_F(ctl_size) },
	{ "style",		T_STRLIST,	_F(ctl_style) },
	{ "text",		T_STR,		_F(label_text) },
	{ NULL,			T_CLOSE,	_F(ctl_done) },
};

static int new_combobox(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->actl.ctl = ldr_getctl(g, &cs->objval)))
		return FFUI_EINVAL;

	if (0 != ffui_combo_createlist(g->actl.ctl, g->wnd))
		return FFUI_ENOMEM;

	state_reset2(g);
	g->resize_flags |= F_RESIZE_CX;
	ffconf_scheme_addctx(cs, combx_args, g);
	return 0;
}


static const ffconf_arg radio_args[] = {
	{ "action",		T_STR,		_F(btn_action) },
	{ "font",		T_OBJ,		_F(label_font) },
	{ "position",	T_INTLIST_S,_F(ctl_pos) },
	{ "resize",		T_STRLIST,	_F(ctl_resize) },
	{ "size",		T_INTLIST,	_F(ctl_size) },
	{ "style",		T_STRLIST,	_F(chbox_style) },
	{ "text",		T_STR,		_F(label_text) },
	{ "tooltip",	T_STR,		_F(ctl_tooltip) },
	{ NULL,			T_CLOSE,	_F(ctl_done) },
};
static int new_radio(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->actl.ctl = ldr_getctl(g, &cs->objval)))
		return FFUI_EINVAL;

	if (0 != ffui_radio_create(g->actl.ctl, g->wnd))
		return FFUI_ENOMEM;

	state_reset(g);
	ffconf_scheme_addctx(cs, radio_args, g);
	return 0;
}


static int tab_style(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (ffstr_eqcz(&val, "multiline"))
		_ffui_style_set(g->actl.tab, TCS_MULTILINE);
	else if (ffstr_eqcz(&val, "fixed-width"))
		_ffui_style_set(g->actl.tab, TCS_FIXEDWIDTH);
	else
		return ctl_style(cs, g, val);
	return 0;
}
static int tab_onchange(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (0 == (g->actl.tab->chsel_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}
static const ffconf_arg tab_args[] = {
	{ "font",		T_OBJ,		_F(label_font) },
	{ "onchange",	T_STR,		_F(tab_onchange) },
	{ "position",	T_INTLIST_S,_F(ctl_pos) },
	{ "resize",		T_STRLIST,	_F(ctl_resize) },
	{ "size",		T_INTLIST,	_F(ctl_size) },
	{ "style",		T_STRLIST,	_F(tab_style) },
	{ NULL,			T_CLOSE,	_F(ctl_done) },
};
static int new_tab(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->actl.tab = ldr_getctl(g, &cs->objval)))
		return FFUI_EINVAL;

	if (0 != ffui_tab_create(g->actl.tab, g->wnd))
		return FFUI_ENOMEM;
	state_reset(g);
	ffconf_scheme_addctx(cs, tab_args, g);
	return 0;
}


enum {
	VIEW_STYLE_CHECKBOXES,
	VIEW_STYLE_EDITLABELS,
	VIEW_STYLE_EXPLORER_THEME,
	VIEW_STYLE_FULL_ROW_SELECT,
	VIEW_STYLE_GRID_LINES,
	VIEW_STYLE_HAS_BUTTONS,
	VIEW_STYLE_HAS_LINES,
	VIEW_STYLE_HORIZ,
	VIEW_STYLE_MULTI_SELECT,
	VIEW_STYLE_TRACK_SELECT,
	VIEW_STYLE_VISIBLE,
};
static const char *const view_styles[] = {
	"checkboxes",
	"edit_labels",
	"explorer_theme",
	"full_row_select",
	"grid_lines",
	"has_buttons",
	"has_lines",
	"horizontal",
	"multi_select",
	"track_select",
	"visible",
};

static int view_style(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (!g->style_reset) {
		g->style_reset = 1;
		// reset to default
		ListView_SetExtendedListViewStyleEx(g->actl.view->h, LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES, 0);
	}

	switch (ffszarr_ifindsorted(view_styles, FF_COUNT(view_styles), val.ptr, val.len)) {

	case VIEW_STYLE_VISIBLE:
		break;

	case VIEW_STYLE_EDITLABELS:
		_ffui_style_set(g->actl.view, LVS_EDITLABELS);
		break;

	case VIEW_STYLE_MULTI_SELECT:
		_ffui_style_clear(g->actl.view, LVS_SINGLESEL);
		break;

	case VIEW_STYLE_GRID_LINES:
		ListView_SetExtendedListViewStyleEx(g->actl.view->h, LVS_EX_GRIDLINES, LVS_EX_GRIDLINES);
		break;

	case VIEW_STYLE_CHECKBOXES:
		ListView_SetExtendedListViewStyleEx(g->actl.view->h, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
		break;

	case VIEW_STYLE_EXPLORER_THEME:
		ffui_view_settheme(g->actl.view);
		break;

	case VIEW_STYLE_HORIZ:
		g->style_horizontal = 1;
		break;

	default:
		return FFUI_EINVAL;
	}
	return 0;
}

static int tview_style(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	switch (ffszarr_ifindsorted(view_styles, FF_COUNT(view_styles), val.ptr, val.len)) {

	case VIEW_STYLE_VISIBLE:
		break;

	case VIEW_STYLE_CHECKBOXES:
		_ffui_style_set(g->actl.view, TVS_CHECKBOXES);
		break;

	case VIEW_STYLE_EXPLORER_THEME:
		ffui_view_settheme(g->actl.view);
#if FF_WIN >= 0x0600
		TreeView_SetExtendedStyle(g->actl.view->h, TVS_EX_FADEINOUTEXPANDOS, TVS_EX_FADEINOUTEXPANDOS);
#endif
		break;

	case VIEW_STYLE_FULL_ROW_SELECT:
		_ffui_style_set(g->actl.view, TVS_FULLROWSELECT);
		break;

	case VIEW_STYLE_TRACK_SELECT:
		_ffui_style_set(g->actl.view, TVS_TRACKSELECT);
		break;

	case VIEW_STYLE_HAS_LINES:
		_ffui_style_set(g->actl.view, TVS_HASLINES);
		break;

	case VIEW_STYLE_HAS_BUTTONS:
		_ffui_style_set(g->actl.view, TVS_HASBUTTONS);
		break;

	case VIEW_STYLE_HORIZ:
		g->style_horizontal = 1;
		break;

	default:
		return FFUI_EINVAL;
	}
	return 0;
}

static int view_color(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	uint clr;

	if ((uint)-1 == (clr = ffpic_color3(val.ptr, val.len, ffpic_clr_a)))
		return FFUI_EINVAL;

	if (!ffsz_cmp(cs->arg->name, "color"))
		ffui_view_clr_text(g->actl.view, clr);
	else
		ffui_view_clr_bg(g->actl.view, clr);
	return 0;
}

static int view_popup_menu(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	ffui_menu *m = g->getctl(g->udata, &val);
	if (m == NULL)
		return FFUI_EINVAL;
	g->actl.view->pmenu = m;
	return 0;
}

static int view_chsel(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (0 == (g->actl.view->chsel_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}

static int view_lclick(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	int id = g->getcmd(g->udata, &val);
	if (id == 0) return FFUI_EINVAL;

	if (!ffsz_cmp(cs->arg->name, "lclick"))
		g->actl.view->lclick_id = id;
	else
		g->actl.view->check_id = id;
	return 0;
}

static int view_double_click(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (0 == (g->actl.view->dblclick_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}


static int viewcol_width(ffconf_scheme *cs, ffui_loader *g, ffint64 val)
{
	ffui_viewcol_setwidth(&g->vicol, val);
	return 0;
}

static int viewcol_align(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	uint a;
	if (ffstr_eqcz(&val, "left"))
		a = HDF_LEFT;
	else if (ffstr_eqcz(&val, "right"))
		a = HDF_RIGHT;
	else if (ffstr_eqcz(&val, "center"))
		a = HDF_CENTER;
	else
		return FFUI_EINVAL;
	ffui_viewcol_setalign(&g->vicol, a);
	return 0;
}

static int viewcol_order(ffconf_scheme *cs, ffui_loader *g, ffint64 val)
{
	ffui_viewcol_setorder(&g->vicol, val);
	return 0;
}

static int viewcol_done(ffconf_scheme *cs, ffui_loader *g)
{
	ffui_view_inscol(g->actl.view, ffui_view_ncols(g->actl.view), &g->vicol);
	return 0;
}
static const ffconf_arg viewcol_args[] = {
	{ "align",		T_STR,		_F(viewcol_align) },
	{ "order",		T_INT32,	_F(viewcol_order) },
	{ "width",		T_INT32,	_F(viewcol_width) },
	{ NULL,			T_CLOSE,	_F(viewcol_done) },
};

static int view_column(ffconf_scheme *cs, ffui_loader *g)
{
	ffstr *name = ffconf_scheme_objval(cs);
	ffui_viewcol_reset(&g->vicol);
	ffui_viewcol_setwidth(&g->vicol, 100);
	ffstr s = vars_val(&g->vars, *name);
	ffui_viewcol_settext(&g->vicol, s.ptr, s.len);
	state_reset(g);
	ffconf_scheme_addctx(cs, viewcol_args, g);
	return 0;
}

static const ffconf_arg view_args[] = {
	{ "bgcolor",	T_STR,		_F(view_color) },
	{ "chsel",		T_STR,		_F(view_chsel) },
	{ "color",		T_STR,		_F(view_color) },
	{ "column",		T_OBJMULTI,	_F(view_column) },
	{ "double_click",T_STR,		_F(view_double_click) },
	{ "lclick",		T_STR,		_F(view_lclick) },
	{ "oncheck",	T_STR,		_F(view_lclick) },
	{ "popup_menu",	T_STR,		_F(view_popup_menu) },
	{ "position",	T_INTLIST_S,_F(ctl_pos) },
	{ "resize",		T_STRLIST,	_F(ctl_resize) },
	{ "size",		T_INTLIST,	_F(ctl_size) },
	{ "style",		T_STRLIST,	_F(view_style) },
	{ NULL,			T_CLOSE,	_F(ctl_done) },
};

static int new_listview(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->actl.ctl = ldr_getctl(g, &cs->objval)))
		return FFUI_EINVAL;

	if (0 != ffui_view_create(g->actl.view, g->wnd))
		return FFUI_ENOMEM;

	state_reset(g);
	g->r.cx = 200;
	g->r.cy = 200;
	g->auto_pos = 1;
	g->resize_flags |= F_RESIZE_CX | F_RESIZE_CY;
	ffconf_scheme_addctx(cs, view_args, g);
	return 0;
}

static const ffconf_arg tview_args[] = {
	{ "bgcolor",	T_STR,		_F(view_color) },
	{ "chsel",		T_STR,		_F(view_chsel) },
	{ "color",		T_STR,		_F(view_color) },
	{ "popup_menu",	T_STR,		_F(view_popup_menu) },
	{ "position",	T_INTLIST_S,_F(ctl_pos) },
	{ "size",		T_INTLIST,	_F(ctl_size) },
	{ "style",		T_STRLIST,	_F(tview_style) },
	{ NULL,			T_CLOSE,	_F(ctl_done) },
};
static int new_treeview(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->actl.ctl = ldr_getctl(g, &cs->objval)))
		return FFUI_EINVAL;

	if (0 != ffui_tree_create(g->actl.view, g->wnd))
		return FFUI_ENOMEM;

	state_reset(g);
	g->r.cx = 200;
	g->r.cy = 200;
	ffconf_scheme_addctx(cs, tview_args, g);
	return 0;
}


static int pnchild_resize(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (ffstr_eqcz(&val, "cx"))
		g->paned->items[g->paned_idx - 1].cx = 1;
	else if (ffstr_eqcz(&val, "cy"))
		g->paned->items[g->paned_idx - 1].cy = 1;
	else
		return FFUI_EINVAL;
	return 0;
}

static int pnchild_move(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (ffstr_eqcz(&val, "x"))
		g->paned->items[g->paned_idx - 1].x = 1;
	else if (ffstr_eqcz(&val, "y"))
		g->paned->items[g->paned_idx - 1].y = 1;
	else
		return FFUI_EINVAL;
	return 0;
}
static const ffconf_arg paned_child_args[] = {
	{ "move",		T_STRLIST,	_F(pnchild_move) },
	{ "resize",		T_STRLIST,	_F(pnchild_resize) },
	{}
};

static int paned_child(ffconf_scheme *cs, ffui_loader *g)
{
	if (g->paned_idx == FF_COUNT(g->paned->items))
		return FFUI_EINVAL;

	void *ctl = ldr_getctl(g, &cs->objval);
	if (ctl == NULL) return FFUI_EINVAL;

	g->paned->items[g->paned_idx++].it = ctl;
	state_reset(g);
	ffconf_scheme_addctx(cs, paned_child_args, g);
	return 0;
}
static const ffconf_arg paned_args[] = {
	{ "child",	T_OBJMULTI,	_F(paned_child) },
	{}
};

static int new_paned(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->paned = ldr_getctl(g, &cs->objval)))
		return FFUI_EINVAL;

	ffmem_zero_obj(g->paned);
	ffui_paned_create(g->paned, g->wnd);

	g->paned_idx = 0;
	state_reset(g);
	ffconf_scheme_addctx(cs, paned_args, g);
	return 0;
}


// DIALOG
static int dlg_title(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	ffui_dlg_title(g->dlg, val.ptr, val.len);
	return 0;
}

static int dlg_filter(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	ffui_dlg_filter(g->dlg, val.ptr, val.len);
	return 0;
}
static const ffconf_arg dlg_args[] = {
	{ "filter",	T_STR,	_F(dlg_filter) },
	{ "title",	T_STR,	_F(dlg_title) },
	{}
};
static int new_dlg(ffconf_scheme *cs, ffui_loader *g)
{
	if (NULL == (g->dlg = g->getctl(g->udata, &cs->objval)))
		return FFUI_EINVAL;
	ffui_dlg_init(g->dlg);
	state_reset(g);
	ffconf_scheme_addctx(cs, dlg_args, g);
	return 0;
}


static int wnd_title(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	val = vars_val(&g->vars, val);
	ffui_settextstr(g->wnd, &val);
	return 0;
}

static int wnd_position(ffconf_scheme *cs, ffui_loader *g, ffint64 v)
{
	int *i = &g->r.x;
	if (g->list_idx == 4)
		return FFUI_EINVAL;
	i[g->list_idx] = (int)v;
	if (g->list_idx == 3) {
		ffui_pos_limit(&g->r, &g->screen);
		ffui_setposrect(g->wnd, &g->r, 0);
		g->wnd_pos = g->r;
	}
	g->list_idx++;
	return 0;
}

static int wnd_placement(ffconf_scheme *cs, ffui_loader *g, ffint64 v)
{
	int li = g->list_idx++;

	if (li == 0) {
		g->wnd_show_code = v;
		return 0;
	} else if (li == 5)
		return FFUI_EINVAL;

	int *i = &g->r.x;
	i[li - 1] = (int)v;

	if (li == 4) {
		ffui_pos_limit(&g->r, &g->screen);
		ffui_wnd_setplacement(g->wnd, g->wnd_show_code, &g->r);
	}
	return 0;
}

static int wnd_icon(ffconf_scheme *cs, ffui_loader *g)
{
	ffmem_zero(&g->ico, sizeof(_ffui_ldr_icon_t));
	g->ico.ldr = g;
	g->ico.load_small = 1;
	state_reset(g);
	ffconf_scheme_addctx(cs, icon_args, &g->ico);
	return 0;
}

/** 'percent': Opacity value, 10-100 */
static int wnd_opacity(ffconf_scheme *cs, ffui_loader *g, ffint64 val)
{
	uint percent = (uint)val;

	if (!(percent >= 10 && percent <= 100))
		return FFUI_EINVAL;

	ffui_wnd_opacity(g->wnd, percent);
	return 0;
}

static int wnd_borderstick(ffconf_scheme *cs, ffui_loader *g, ffint64 val)
{
	g->wnd->bordstick = (byte)val;
	return 0;
}

static int wnd_style(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (ffstr_eqcz(&val, "visible"))
		g->wnd_visible = 1;
	else
		return FFUI_EINVAL;
	return 0;
}

static int wnd_bgcolor(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	uint clr;

	if (ffstr_eqcz(&val, "null"))
		clr = GetSysColor(COLOR_BTNFACE);
	else if ((uint)-1 == (clr = ffpic_color3(val.ptr, val.len, ffpic_clr_a)))
		return FFUI_EINVAL;
	ffui_wnd_bgcolor(g->wnd, clr);
	return 0;
}

static int wnd_color(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	uint clr;
	 if ((uint)-1 == (clr = ffpic_color3(val.ptr, val.len, ffpic_clr_a)))
		return FFUI_EINVAL;
	g->wnd->color = clr;
	return 0;
}

static int wnd_onclose(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	if (0 == (g->wnd->onclose_id = g->getcmd(g->udata, &val)))
		return FFUI_EINVAL;
	return 0;
}

static int wnd_popupfor(ffconf_scheme *cs, ffui_loader *g, ffstr val)
{
	ffui_ctl *parent = g->getctl(g->udata, &val);
	if (parent == NULL) return FFUI_EINVAL;

	SetWindowLongPtr(g->wnd->h, GWLP_HWNDPARENT, (LONG_PTR)parent->h);
	ffui_wnd_setpopup(g->wnd);
	return 0;
}

static int wnd_done(ffconf_scheme *cs, ffui_loader *g)
{
	if (g->ico.icon.h != NULL) {
		ffui_wnd_seticon(g->wnd, &g->ico.icon, &g->ico.icon_small);

	} else {
		ffui_window *parent;
		if (NULL != (parent = ffui_ctl_parent(g->wnd))) {
			ffui_icon ico, ico_small;
			ffui_wnd_icon(parent, &ico, &ico_small);
			ffui_wnd_seticon(g->wnd, &ico, &ico_small);
		}
	}

	{
	//main menu isn't visible until the window is resized
	HMENU mm = GetMenu(g->wnd->h);
	if (mm != NULL)
		SetMenu(g->wnd->h, mm);
	}

	if (g->accels.len != 0) {
		int r = ffui_wnd_hotkeys(g->wnd, (void*)g->accels.ptr, g->accels.len);
		g->accels.len = 0;
		if (r != 0)
			return FFUI_ESYS;
	}

	if ((uint)g->wnd_pos.cx < g->edge_right
		|| (uint)g->wnd_pos.cy - 40 < g->edge_bottom) {
		// Increase the window's width+height so that all controls are fully visible
		g->wnd_pos.cx = g->edge_right + 2;
		g->wnd_pos.cy = 40 + g->edge_bottom + 2;
		ffui_setposrect(g->wnd, &g->wnd_pos, 0);
	}

	if (g->wnd_visible) {
		g->wnd_visible = 0;
		ffui_show(g->wnd, 1);
	}

	g->wnd = NULL;
	ffmem_free(g->wnd_name);  g->wnd_name = NULL;
	return 0;
}
static const ffconf_arg wnd_args[] = {
	{ "bgcolor",	T_STR,		_F(wnd_bgcolor) },
	{ "borderstick",FFCONF_TINT8,_F(wnd_borderstick) },
	{ "button",		T_OBJMULTI,	_F(new_button) },
	{ "checkbox",	T_OBJMULTI,	_F(new_checkbox) },
	{ "color",		T_STR,		_F(wnd_color) },
	{ "combobox",	T_OBJMULTI,	_F(new_combobox) },
	{ "editbox",	T_OBJMULTI,	_F(new_editbox) },
	{ "font",		T_OBJ,		_F(label_font) },
	{ "icon",		T_OBJ,		_F(wnd_icon) },
	{ "image",		T_OBJMULTI,	_F(new_image) },
	{ "label",		T_OBJMULTI,	_F(new_label) },
	{ "listview",	T_OBJMULTI,	_F(new_listview) },
	{ "mainmenu",	T_OBJ,		_F(new_mmenu) },
	{ "on_close",	T_STR,		_F(wnd_onclose) },
	{ "opacity",	T_INT32,	_F(wnd_opacity) },
	{ "paned",		T_OBJMULTI,	_F(new_paned) },
	{ "placement",	T_INTLIST_S,_F(wnd_placement) },
	{ "popupfor",	T_STR,		_F(wnd_popupfor) },
	{ "position",	T_INTLIST_S,_F(wnd_position) },
	{ "progressbar",T_OBJMULTI,	_F(new_pgsbar) },
	{ "radiobutton",T_OBJMULTI,	_F(new_radio) },
	{ "statusbar",	T_OBJ,		_F(new_stbar) },
	{ "style",		T_STRLIST,	_F(wnd_style) },
	{ "tab",		T_OBJMULTI,	_F(new_tab) },
	{ "text",		T_OBJMULTI,	_F(new_editbox) },
	{ "title",		T_STR,		_F(wnd_title) },
	{ "trackbar",	T_OBJMULTI,	_F(new_trkbar) },
	{ "trayicon",	T_OBJ,		_F(new_tray) },
	{ "treeview",	T_OBJMULTI,	_F(new_treeview) },
	{ NULL,			T_CLOSE,	_F(wnd_done) },
};

static void _ffui_wnd_dark_title_set(ffui_window *w)
{
	ffdl dl;
	if (FFDL_NULL == (dl = ffdl_open("dwmapi.dll", 0)))
		return;

	typedef void (*DwmSetWindowAttribute_t)(void*, DWORD, void*, DWORD);
	DwmSetWindowAttribute_t _DwmSetWindowAttribute;
	if (NULL == (_DwmSetWindowAttribute = ffdl_addr(dl, "DwmSetWindowAttribute")))
		goto end;

	const uint _DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
	BOOL value = 1;
	_DwmSetWindowAttribute(w->h, _DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));

end:
	ffdl_close(dl);
}

static int new_wnd(ffconf_scheme *cs, ffui_loader *g)
{
	ffui_window *wnd;

	if (NULL == (wnd = g->getctl(g->udata, &cs->objval)))
		return FFUI_EINVAL;

	uint off = FF_OFF(ffui_loader, wnd);
	ffmem_zero((byte*)g + off, sizeof(ffui_loader) - off);

	g->wnd = wnd;
	if (NULL == (g->wnd_name = ffsz_dupn(cs->objval.ptr, cs->objval.len)))
		return FFUI_ENOMEM;
	g->actl.ctl = (ffui_ctl*)wnd;
	if (0 != ffui_wnd_create(wnd))
		return FFUI_ENOMEM;

	if (g->dark_mode && !g->dark_title_set) {
		g->dark_title_set = 1;
		_ffui_wnd_dark_title_set(g->wnd);
	}

	ffconf_scheme_addctx(cs, wnd_args, g);
	return 0;
}


// LANGUAGE
static int inc_lang_entry(ffconf_scheme *cs, ffui_loader *g, ffstr fn)
{
	const ffstr *lang = ffconf_scheme_keyname(cs);
	if (!(ffstr_eqz(lang, "default")
		|| ffstr_eq(lang, g->language, 2)))
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
		if (ffstr_eqz(lang, "default"))
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
	{ "*",	T_STRMULTI,	_F(inc_lang_entry) },
	{}
};
static int inc_lang(ffconf_scheme *cs, ffui_loader *g)
{
	ffconf_scheme_addctx(cs, inc_lang_args, g);
	return 0;
}


static const ffconf_arg top_args[] = {
	{ "dialog",	T_OBJMULTI,	_F(new_dlg) },
	{ "include_language",	T_OBJ,	_F(inc_lang) },
	{ "menu",	T_OBJMULTI,	_F(new_menu) },
	{ "window",	T_OBJMULTI,	_F(new_wnd) },
	{}
};

void ffui_ldr_init(ffui_loader *g, ffui_ldr_getctl_t getctl, ffui_ldr_getcmd_t getcmd, void *udata)
{
	ffmem_zero_obj(g);
	ffui_screenarea(&g->screen);
	vars_init(&g->vars);
	g->getctl = getctl;
	g->getcmd = getcmd;
	g->udata = udata;
	vars_init(&g->vars);
}

void ffui_ldr_fin(ffui_loader *g)
{
	vars_free(&g->vars);
	ffvec_free(&g->lang_data);
	ffvec_free(&g->lang_data_def);
	// g->paned_array
	ffvec_free(&g->accels);
	ffmem_free(g->errstr);  g->errstr = NULL;
	ffmem_free(g->wnd_name);  g->wnd_name = NULL;
}

static void* ldr_getctl(ffui_loader *g, const ffstr *name)
{
	char buf[255];
	ffstr s;
	s.ptr = buf;
	s.len = ffs_format_r0(buf, sizeof(buf), "%s.%S", g->wnd_name, name);
	return g->getctl(g->udata, &s);
}

int ffui_ldr_load(ffui_loader *g, ffstr data)
{
	ffstr errstr = {};
	int r = ffconf_parse_object(top_args, g, &data, 0, &errstr);
	if (r != 0) {
		ffmem_free(g->errstr);
		g->errstr = ffsz_dupstr(&errstr);
	}

	ffstr_free(&errstr);
	return r;
}

int ffui_ldr_loadfile(ffui_loader *g, const char *fn)
{
	ffpath_splitpath(fn, ffsz_len(fn), &g->path, NULL);
	g->path.len += FFS_LEN("/");

	ffstr errstr = {};
	int r = ffconf_parse_file(top_args, g, fn, 0, &errstr, 1*1024*1024);
	if (r != 0) {
		ffmem_free(g->errstr);
		g->errstr = ffsz_dupstr(&errstr);
	}

	ffstr_free(&errstr);
	return r;
}

void ffui_ldr_loadconf(ffui_loader *g, ffstr data)
{
	struct ffconf_obj conf = {};
	ffconf_scheme cs = {};
	ffstr line, ctx;

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

		if (NULL == (g->actl.ctl = g->getctl(g->udata, &ctx)))
			continue; // couldn't find the control by path "ctx0[.ctx1]"

		state_reset(g);

		switch (g->actl.ctl->uid) {
		case FFUI_UID_WINDOW:
			g->wnd = (void*)g->actl.ctl;
			ffconf_scheme_addctx(&cs, wnd_args, g);  break;

		case FFUI_UID_EDITBOX:
			ffconf_scheme_addctx(&cs, editbox_args, g);  break;

		case FFUI_UID_LABEL:
			ffconf_scheme_addctx(&cs, label_args, g);  break;

		case FFUI_UID_COMBOBOX:
			ffconf_scheme_addctx(&cs, combx_args, g);  break;

		case FFUI_UID_TRACKBAR:
			ffconf_scheme_addctx(&cs, trkbar_args, g);  break;

		case FFUI_UID_LISTVIEW:
			ffconf_scheme_addctx(&cs, view_args, g);  break;

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

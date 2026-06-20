/**
Not implemented:
* combo box

2026, Simon Zolin */

#define UNICODE
#include "dark-theme.h"
#include <uxtheme.h>
#include <vsstyle.h>

typedef void (WINAPI *AllowDarkModeForWindow_t)(HWND, int);
typedef void (WINAPI *SetPreferredAppMode_t)(int);
typedef void (WINAPI *DwmSetWindowAttribute_t)(void*, DWORD, void*, DWORD);

static int dkth_os_ver()
{
	HMODULE ntdll;
	if (!(ntdll = GetModuleHandleW(L"ntdll.dll")))
		return -1;
	typedef NTSTATUS (WINAPI *RtlGetVersion_t)(void*);
	RtlGetVersion_t _RtlGetVersion;
	if (!(_RtlGetVersion = (RtlGetVersion_t)(void*)GetProcAddress(ntdll, "RtlGetVersion")))
		return -1;
	RTL_OSVERSIONINFOEXW osvi = {
		.dwOSVersionInfoSize = sizeof(osvi)
	};
	if (0 != _RtlGetVersion(&osvi))
		return -1;
	return osvi.dwMajorVersion;
}

int dark_theme_init(struct dark_theme *t, unsigned flags)
{
	if (dkth_os_ver() < 10)
		return -1;

	HMODULE uxtheme;
	if ((uxtheme = LoadLibraryExW(L"uxtheme.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32))) {
		t->_AllowDarkModeForWindow = GetProcAddress(uxtheme, MAKEINTRESOURCEA(133));
		t->_SetPreferredAppMode = GetProcAddress(uxtheme, MAKEINTRESOURCEA(135));
	}

	HMODULE dwmapi;
	if ((dwmapi = LoadLibraryExW(L"dwmapi.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32))) {
		t->_DwmSetWindowAttribute = GetProcAddress(dwmapi, "DwmSetWindowAttribute");
	}

	return !(t->_AllowDarkModeForWindow
		&& t->_SetPreferredAppMode
		&& t->_DwmSetWindowAttribute);
}

#define dkth_subclass(h, func, t) \
	SetWindowSubclass(h, func, (UINT_PTR)1, (DWORD_PTR)(t))

LRESULT WINAPI dark_theme_wnd_proc(struct dark_theme *t, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORSTATIC: {
		HDC hdc = (HDC)wParam;
		SetTextColor(hdc, t->text);
		SetBkMode(hdc, TRANSPARENT);
		return (LRESULT)t->window_bg_br;
	}

	case WM_ERASEBKGND: {
		HDC hdc = (HDC)wParam;
		RECT rect;
		GetClientRect(hWnd, &rect);
		FillRect(hdc, &rect, t->window_bg_br);
		return 1;
	}
	}

	return -1;
}

static LRESULT WINAPI dkth_window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	LRESULT r = dark_theme_wnd_proc((struct dark_theme*)dwRefData, hWnd, uMsg, wParam, lParam);
	if (r != -1)
		return r;
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

enum {
	WM_UAHDRAWMENU = 0x0091,
	WM_UAHDRAWMENUITEM,
};

struct UAHMENU {
	HMENU hmenu;
	HDC hdc;
	DWORD dwFlags;
};

struct UAHMENUITEM {
	int iPosition;
};

struct UAHDRAWMENUITEM {
	DRAWITEMSTRUCT dis;
	struct UAHMENU m;
	struct UAHMENUITEM mi;
};

static LRESULT WINAPI dkth_mmenu_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	struct dark_theme *t = (void*)dwRefData;

	switch (uMsg) {
	case WM_UAHDRAWMENU: {
		struct UAHMENU *m = (void*)lParam;

		MENUBARINFO mbi = { sizeof(mbi) };
		GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);

		// Screen area -> client area
		RECT r = mbi.rcBar, r_wnd;
		GetWindowRect(hWnd, &r_wnd);
		r.left -= r_wnd.left;
		r.top -= r_wnd.top;
		r.right -= r_wnd.left;
		r.bottom -= r_wnd.top;

		FillRect(m->hdc, &r, t->window_bg_br);
		return 1;
	}

	case WM_UAHDRAWMENUITEM: {
		struct UAHDRAWMENUITEM *dmi = (void*)lParam;

		// Background
		HBRUSH br = (dmi->dis.itemState & (ODS_HOTLIGHT | ODS_SELECTED)) ? t->menu_light_br
			: t->window_bg_br;
		FillRect(dmi->m.hdc, &dmi->dis.rcItem, br);

		// Get text
		wchar_t buf[256];
		MENUITEMINFOW mii = {
			.cbSize = sizeof(mii),
			.fMask = MIIM_STRING,
			.dwTypeData = buf,
			.cch = sizeof(buf) / 2 - 1,
		};
		GetMenuItemInfoW(dmi->m.hmenu, dmi->mi.iPosition, 1, &mii);

		// Draw text
		unsigned flags = DT_CENTER | DT_SINGLELINE | DT_VCENTER
			| ((dmi->dis.itemState & ODS_NOACCEL) ? DT_HIDEPREFIX : 0);
		DTTOPTS opts = {
			.dwSize = sizeof(opts),
			.dwFlags = DTT_TEXTCOLOR,
			.crText = t->text_alt,
		};
		DrawThemeTextEx(t->thmenu, dmi->m.hdc, MENU_BARITEM, MBI_NORMAL, buf, mii.cch, flags, &dmi->dis.rcItem, &opts);
		return 1;
	}

	case WM_NCACTIVATE:
	case WM_NCPAINT: {
		LRESULT res = DefSubclassProc(hWnd, uMsg, wParam, lParam);

		RECT r_wnd;
		GetWindowRect(hWnd, &r_wnd);

		POINT pt = {};
		ClientToScreen(hWnd, &pt);
		int cl_y_off = pt.y - r_wnd.top;

		RECT r = {
			.left = 0,
			.top = cl_y_off - 1,
			.right = r_wnd.right - r_wnd.left,
			.bottom = cl_y_off,
		};

		// Draw the line under the main menu
		HDC hdc = GetWindowDC(hWnd);
		FillRect(hdc, &r, t->window_bg_br);
		ReleaseDC(hWnd, hdc);

		return res;
	}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static LRESULT WINAPI dkth_tab_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	struct dark_theme *t = (void*)dwRefData;

	switch (uMsg) {
	case WM_PAINT: {
		POINT cursor;
		GetCursorPos(&cursor);
		ScreenToClient(hWnd, &cursor);

		RECT r, rs[16], *rr = rs;
		unsigned n = TabCtrl_GetItemCount(hWnd);
		int i_sel = TabCtrl_GetCurSel(hWnd);
		int i_hl = -1;
		HFONT font = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
		HANDLE heap = NULL;
		if (n > sizeof(rs) / sizeof(rs[0])) {
			heap = GetProcessHeap();
			if (!(rr = HeapAlloc(heap, 0, sizeof(RECT) * n)))
				break;
		}

		for (unsigned i = 0;  i < n;  i++) {
			TabCtrl_GetItemRect(hWnd, i, &rr[i]);
			if (i != (unsigned)i_sel && PtInRect(&rr[i], cursor))
				i_hl = i;
		}

		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		IntersectClipRect(hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);

		// Background
		GetClientRect(hWnd, &r);
		FillRect(hdc, &r, t->window_bg_br);
		if (i_sel >= 0) {
			FillRect(hdc, &rr[i_sel], t->tab_sel_br);
			if (t->tab_frame_br)
				FrameRect(hdc, &rr[i_sel], t->tab_frame_br);
		}
		if (i_hl >= 0)
			FillRect(hdc, &rr[i_hl], t->tab_light_br);

		// Text
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, t->text_alt);
		SelectObject(hdc, font);

		for (unsigned i = 0;  i < n;  i++) {
			wchar_t title[256];
			TCITEMW tci = {
				.mask = TCIF_TEXT,
				.pszText = title,
				.cchTextMax = sizeof(title) / 2 - 1,
			};
			TabCtrl_GetItem(hWnd, i, &tci);
			DrawTextW(hdc, title, -1, &rr[i], DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}

		if (rr != rs)
			HeapFree(heap, 0, rr);
		EndPaint(hWnd, &ps);
		return 0;
	}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static LRESULT WINAPI dkth_listview_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	const struct dark_theme *t = (struct dark_theme*)dwRefData;
	switch (uMsg) {
	case WM_NOTIFY:
		if (((NMHDR*)lParam)->code == NM_CUSTOMDRAW) {
			const NMCUSTOMDRAW *nmcd = (NMCUSTOMDRAW*)lParam;
			switch (nmcd->dwDrawStage) {
			case CDDS_PREPAINT:
				return CDRF_NOTIFYITEMDRAW;

			case CDDS_ITEMPREPAINT:
				SetTextColor(nmcd->hdc, t->listview_header);
				return CDRF_NEWFONT;

			default:
				return CDRF_DODEFAULT;
			}
		}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static int dkth_listview(struct dark_theme *t, HWND h)
{
	if (t->_AllowDarkModeForWindow) {
		HWND lvh = ListView_GetHeader(h);
		((AllowDarkModeForWindow_t)t->_AllowDarkModeForWindow)(lvh, 1);
		SetWindowTheme(lvh, L"ItemsView", NULL);

		((AllowDarkModeForWindow_t)t->_AllowDarkModeForWindow)(h, 1);
		SetWindowTheme(h, L"Explorer", NULL);
	}

	dkth_subclass(h, dkth_listview_proc, t);
	ListView_SetBkColor(h, t->listview_bg);
	ListView_SetTextBkColor(h, t->listview_bg);
	ListView_SetTextColor(h, t->listview_text);
	return 0;
}

static LRESULT WINAPI dkth_stbar_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	struct dark_theme *t = (void*)dwRefData;

	switch (uMsg) {
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		HFONT font = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
		SelectObject(hdc, font);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, t->text_alt);

		unsigned n = SendMessageW(hWnd, SB_GETPARTS, 0, 0);
		for (unsigned i = 0;  i < n;  i++) {
			RECT r;
			SendMessageW(hWnd, SB_GETRECT, i, (LPARAM)&r);

			unsigned len = SendMessageW(hWnd, SB_GETTEXTLENGTHW, i, 0);
			wchar_t buf[256];
			if (len + 1 <= sizeof(buf) / 2) {
				SendMessageW(hWnd, SB_GETTEXTW, i, (LPARAM)buf);
			} else {
				len = 3;
				buf[0] = buf[1] = buf[2] = L'.';
			}
			DrawTextW(hdc, buf, len, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		}

		EndPaint(hWnd, &ps);
		return 0;
	}

	case WM_ERASEBKGND: {
		RECT r;
		GetClientRect(hWnd, &r);
		FillRect((HDC)wParam, &r, t->window_bg_br);
		return 1;
	}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

int dark_theme_ctl(struct dark_theme *t, unsigned flags, HWND h)
{
	if (!t)
		return 1;

	switch (flags & 0xff) {
	case DARK_THEME_QUERY: {
		DWORD data, cap = sizeof(data);
		return (!RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", RRF_RT_REG_DWORD, NULL, &data, &cap)
			&& data == 0);
	}

	case DARK_THEME_APP:
		if (t->_SetPreferredAppMode) {
			((SetPreferredAppMode_t)t->_SetPreferredAppMode)(2);
			return 0;
		}
		break;

	case DARK_THEME_WINDOW_TITLE:
		if (t->_DwmSetWindowAttribute) {
			const unsigned _DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
			BOOL value = 1;
			((DwmSetWindowAttribute_t)t->_DwmSetWindowAttribute)(h, _DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
			return 0;
		}
		break;

	case DARK_THEME_WINDOW:
		if (!t->window_bg_br)
			t->window_bg_br = CreateSolidBrush(t->background);
		dkth_subclass(h, dkth_window_proc, t);
		return 0;

	case DARK_THEME_WINDOW_MAIN_MENU:
		if (!t->thmenu)
			t->thmenu = OpenThemeData(h, L"Menu");
		if (!t->menu_light_br)
			t->menu_light_br = CreateSolidBrush(t->menu_bg_light);
		dkth_subclass(h, dkth_mmenu_proc, t);
		return 0;

	case DARK_THEME_BUTTON:
	case DARK_THEME_EDIT:
		SetWindowTheme(h, L"DarkMode_Explorer", NULL);
		return 0;

	case DARK_THEME_CHECKBOX:
		SetWindowTheme(h, L"", L"");
		return 0;

	case DARK_THEME_TAB:
		if (!t->tab_light_br)
			t->tab_light_br = CreateSolidBrush(t->tab_bg_light);
		if (!t->tab_sel_br)
			t->tab_sel_br = CreateSolidBrush(t->tab_bg_sel);
		if (!t->tab_frame_br && t->tab_frame_sel != t->tab_bg_sel)
			t->tab_frame_br = CreateSolidBrush(t->tab_frame_sel);
		dkth_subclass(h, dkth_tab_proc, t);
		return 0;

	case DARK_THEME_LISTVIEW:
		return dkth_listview(t, h);

	case DARK_THEME_STATUSBAR:
		dkth_subclass(h, dkth_stbar_proc, t);
		return 0;
	}

	return -1;
}

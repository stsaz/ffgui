/**
Not implemented:
* tabs
* button
* combo box
* multiline edit scrollbars
* status bar

2026, Simon Zolin */

#include <winnt.h>
#include <vsstyle.h>
#include "dark-theme.h"
#include "ffsys/std.h"

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
	if ((uxtheme = LoadLibraryExW(L"uxtheme.dll", NULL, 0))) {
		t->_ShouldAppsUseDarkMode = (ShouldAppsUseDarkMode_t)(void*)GetProcAddress(uxtheme, MAKEINTRESOURCEA(132));
		t->_AllowDarkModeForWindow = (AllowDarkModeForWindow_t)(void*)GetProcAddress(uxtheme, MAKEINTRESOURCEA(133));
		t->_SetPreferredAppMode = (SetPreferredAppMode_t)(void*)GetProcAddress(uxtheme, MAKEINTRESOURCEA(135));
	}

	HMODULE dwmapi;
	if ((dwmapi = LoadLibraryExW(L"dwmapi.dll", NULL, 0))) {
		t->_DwmSetWindowAttribute = (DwmSetWindowAttribute_t)(void*)GetProcAddress(dwmapi, "DwmSetWindowAttribute");
	}

	return (!t->_ShouldAppsUseDarkMode
		|| !t->_AllowDarkModeForWindow
		|| !t->_SetPreferredAppMode
		|| !t->_DwmSetWindowAttribute);
}

#define dkth_subclass(h, func, t) \
	SetWindowSubclass(h, func, (UINT_PTR)1, (DWORD_PTR)(t))

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
		t->_AllowDarkModeForWindow(lvh, 1);
		SetWindowTheme(lvh, L"ItemsView", NULL);

		t->_AllowDarkModeForWindow(h, 1);
		SetWindowTheme(h, L"Explorer", NULL);
	}

	dkth_subclass(h, dkth_listview_proc, t);
	SendMessageW(h, LVM_SETBKCOLOR, 0, t->listview_bg);
	SendMessageW(h, LVM_SETTEXTBKCOLOR, 0, t->listview_bg);
	SendMessageW(h, LVM_SETTEXTCOLOR, 0, t->listview_text);
	return 0;
}

LRESULT WINAPI dark_theme_wnd_proc(struct dark_theme *t, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORSTATIC: {
		HDC hdc = (HDC)wParam;
		SetTextColor(hdc, t->text);
		SetBkMode(hdc, TRANSPARENT);
		return (LRESULT)t->wbr;
	}

	case WM_ERASEBKGND: {
		HDC hdc = (HDC)wParam;
		RECT rect;
		GetClientRect(hWnd, &rect);
		FillRect(hdc, &rect, t->wbr);
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

		FillRect(m->hdc, &r, t->wbr);
		return 1;
	}

	case WM_UAHDRAWMENUITEM: {
		struct UAHDRAWMENUITEM *dmi = (void*)lParam;

		// Background
		HBRUSH br = (dmi->dis.itemState & (ODS_HOTLIGHT | ODS_SELECTED)) ? t->mmlbr
			: t->wbr;
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
			.crText = t->text,
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
		FillRect(hdc, &r, t->wbr);
		ReleaseDC(hWnd, hdc);

		return res;
	}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

int dark_theme_ctl(struct dark_theme *t, unsigned flags, HWND h)
{
	if (!t)
		return 1;

	switch (flags & 0xff) {
	case DARK_THEME_QUERY:
		if (t->_ShouldAppsUseDarkMode)
			return (t->_ShouldAppsUseDarkMode() & 1);
		break;

	case DARK_THEME_APP:
		if (t->_SetPreferredAppMode) {
			t->_SetPreferredAppMode(1);
			return 0;
		}
		break;

	case DARK_THEME_WINDOW_TITLE:
		if (t->_DwmSetWindowAttribute) {
			const unsigned _DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
			BOOL value = 1;
			t->_DwmSetWindowAttribute(h, _DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
			return 0;
		}
		break;

	case DARK_THEME_WINDOW:
		if (!t->wbr)
			t->wbr = CreateSolidBrush(t->background);
		dkth_subclass(h, dkth_window_proc, t);
		return 0;

	case DARK_THEME_WINDOW_MAIN_MENU:
		if (!t->thmenu)
			t->thmenu = OpenThemeData(h, L"Menu");
		if (!t->mmlbr)
			t->mmlbr = CreateSolidBrush(t->menu_bg_light);
		dkth_subclass(h, dkth_mmenu_proc, t);
		return 0;

	case DARK_THEME_CHECKBOX:
		SetWindowTheme(h, L"", L"");
		return 0;

	case DARK_THEME_LISTVIEW:
		return dkth_listview(t, h);
	}

	return -1;
}

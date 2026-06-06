/** Win32API GUI dark theme
2026, Simon Zolin */

#pragma once
#include <windows.h>

typedef int (WINAPI *ShouldAppsUseDarkMode_t)();
typedef void (WINAPI *AllowDarkModeForWindow_t)(HWND, int);
typedef void (WINAPI *SetPreferredAppMode_t)(int);
typedef void (WINAPI *DwmSetWindowAttribute_t)(void*, DWORD, void*, DWORD);

struct dark_theme {
	// Colors:
	COLORREF background		// Window background
		, text				// Text
		, menu_bg_light		// Menu background (highlight)
		, listview_header	// ListView header text
		, listview_bg		// ListView background
		, listview_text;	// ListView item text

	HBRUSH wbr
		, mmlbr;
	HANDLE thmenu;

	ShouldAppsUseDarkMode_t		_ShouldAppsUseDarkMode;
	AllowDarkModeForWindow_t	_AllowDarkModeForWindow;
	SetPreferredAppMode_t		_SetPreferredAppMode;
	DwmSetWindowAttribute_t		_DwmSetWindowAttribute;
};

enum DARK_THEME {
	DARK_THEME_LIGHT,
	DARK_THEME_DARK,
};

enum DARK_THEME_FLAGS {
	/** Get the system's color theme.
	Return a value from enum DARK_THEME */
	DARK_THEME_QUERY = 1,

	/** Set dark theme for the entire application process */
	DARK_THEME_APP,

	/** Set dark theme for the window's Title Bar */
	DARK_THEME_WINDOW_TITLE,

	/** Apply dark background and customize child control colors (label, checkbox, listbox, editbox) */
	DARK_THEME_WINDOW,

	/** Apply dark background for the window's main menu */
	DARK_THEME_WINDOW_MAIN_MENU,

	/** Set dark theme for a CheckBox control */
	DARK_THEME_CHECKBOX,

	/** Set dark theme for a ListView control */
	DARK_THEME_LISTVIEW,
};

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize dark themed GUI context.
theme: The context object (must be zero-initialized)
flags: Reserved for future use
Return 0 on success, non-zero on error */
int dark_theme_init(struct dark_theme *theme, unsigned flags);

/** Apply theme settings to a specific window or control.
theme: The initialized theme context
flags: A value from enum DARK_THEME_FLAGS
h: The HWND of the target window/control
Return 0 on success, -1 on error */
int dark_theme_ctl(struct dark_theme *theme, unsigned flags, HWND h);

/** Handle window messages.
Note: no need to call this when using DARK_THEME_WINDOW.
Return -1 if the message was not handled */
LRESULT WINAPI dark_theme_wnd_proc(struct dark_theme *t, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
}
#endif

/** Set text and background colors */
static inline void dark_theme_colors(struct dark_theme *t, unsigned text, unsigned background) {
	t->background = t->listview_bg = __builtin_bswap32(background << 8); // "BGR0" -> "0BGR" -> "RGB0"
	t->menu_bg_light = __builtin_bswap32((background + 0x222222) << 8);
	t->text = t->listview_header = t->listview_text = __builtin_bswap32(text << 8);
}

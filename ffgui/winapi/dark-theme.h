/** Win32API GUI dark theme
2026, Simon Zolin */

#pragma once
#include <windows.h>

struct dark_theme {
	// Colors:
	COLORREF background		// Window background
		, text				// Text
		, text_alt			// Text (alternative)
		, menu_bg_light		// Menu background (highlight)
		, tab_bg_light		// Tab background (highlight)
		, tab_bg_sel		// Tab background (selected)
		, tab_frame_sel		// Tab frame (selected)
		, listview_header	// ListView header text
		, listview_bg		// ListView background
		, listview_text;	// ListView item text

	HBRUSH window_bg_br
		, menu_light_br
		, tab_light_br
		, tab_sel_br
		, tab_frame_br;
	HANDLE menu_theme
		, stbar_theme;

	void* _AllowDarkModeForWindow;
	void* _SetPreferredAppMode;
	void* _DwmSetWindowAttribute;
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

	/** Apply "DarkMode_Explorer" theme for the push button */
	DARK_THEME_BUTTON,

	/** Set dark theme for a CheckBox control */
	DARK_THEME_CHECKBOX,

	/** Set dark theme for a RadioButton control */
	DARK_THEME_RADIOBUTTON,

	/** Set dark theme for a ComboBox control */
	DARK_THEME_COMBOBOX,

	/** Apply "DarkMode_Explorer" theme for the edit control */
	DARK_THEME_EDIT,

	/** Set background/text colors for a Tab control */
	DARK_THEME_TAB,

	/** Set dark theme for a ListView control */
	DARK_THEME_LISTVIEW,

	/** Set dark theme for a TreeView control */
	DARK_THEME_TREEVIEW,

	/** Set dark theme for a StatusBar control (subclass) */
	DARK_THEME_STATUSBAR,
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

/** Convert RGB to COLORREF */
static inline unsigned dark_theme_rgb2cr(unsigned rgb) {
	return __builtin_bswap32(rgb << 8); // BGR0 -> 0BGR -> RGB0
}

static inline void dark_theme_colors(struct dark_theme *t, unsigned background, unsigned text) {
	t->background = t->listview_bg = dark_theme_rgb2cr(background);
	t->tab_bg_sel = t->tab_frame_sel = dark_theme_rgb2cr(background + 0x111111);
	t->menu_bg_light = t->tab_bg_light = dark_theme_rgb2cr(background + 0x111111);
	t->text = t->listview_text = dark_theme_rgb2cr(text);
	t->text_alt = t->listview_header = dark_theme_rgb2cr(text - 0x111111);
}

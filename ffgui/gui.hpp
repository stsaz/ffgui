/** C++ cross-platform GUI interface
2023, Simon Zolin */

struct ffui_menuxx : ffui_menu {
	~ffui_menuxx() { ffui_menu_destroy(this); }
	void	check(uint id, bool check) { ffui_menu_check(this, id, check); }
};

struct ffui_labelxx : ffui_label {
	void	text(const char *sz) { ffui_send_label_settext(this, sz); }
#ifdef FF_LINUX
	void	markup(const char *sz) { gtk_label_set_markup(GTK_LABEL(h), sz); }
#endif
};

struct ffui_editxx : ffui_edit {
	ffstr	text() { ffstr s; ffui_send_edit_textstr(this, &s); return s; }
	void	text(const char *sz) { ffstr s = FFSTR_INITZ(sz); ffui_send_edit_settextstr(this, &s); }
	void	text(ffstr s) { ffui_send_edit_settextstr(this, &s); }

	void	select(uint start, uint end) { ffui_edit_sel(this, start, end); }
	void	sel_all() { ffui_edit_selall(this); }

	void	focus() { ffui_ctl_focus(this); }
};

struct ffui_textxx : ffui_text {
	ffstr	text() { ffstr s;  ffui_send_text_text(this, &s);  return s; }
	void	clear() { ffui_text_clear(this); }
	void	add(ffstr s) { ffui_send_text_addtextstr(this, &s); }
};

struct ffui_buttonxx : ffui_button {
	void	text(const char *sz) { ffui_button_settextz(this, sz); }

	void	icon(const ffui_icon& ico) { ffui_send_button_seticon(this, &ico); }

	void	enable(bool val) { ffui_ctl_enable(this, val); }
};

struct ffui_checkboxxx : ffui_checkbox {
	void	check(bool val) { ffui_checkbox_check(this, val); }
	bool	checked() { return ffui_send_checkbox_checked(this); }

	void	text(const char *sz) { ffui_send_checkbox_settextz(this, sz); }
};

struct ffui_comboboxxx : ffui_combobox {
	void	add(const char *text) { ffui_combo_add(this, text); }
	void	clear() { ffui_combo_clear(this); }
	ffstr	text() { return ffui_combo_text_active(this); }
	void	text(const char *text) { ffui_combo_text_set(this, text); }

	void	set(int index) { ffui_combo_set(this, index); }
	int		get() { return ffui_combo_active(this); }
};

struct ffui_trackbarxx : ffui_trackbar {
	void	set(uint value) { ffui_post_track_set(this, value); }
	uint	get() { return ffui_track_val(this); }

	void	range(uint range) { ffui_post_track_range_set(this, range); }
};

struct ffui_tabxx : ffui_tab {
	void	add(const char *sz) { ffui_send_tab_ins(this, sz); }
	void	del(uint i) { ffui_post_tab_del(this, i); }
	void	select(uint i) { ffui_send_tab_setactive(this, i); }
	uint	changed() { return ffui_tab_changed_index(this); }
	uint	count() { return ffui_tab_count(this); }
};

struct ffui_statusbarxx : ffui_statusbar {
	void	text(const char *sz) { ffui_send_status_settextz(this, sz); }
};

struct ffui_viewitemxx : ffui_viewitem {
	ffui_viewitemxx(ffstr s) { ffmem_zero_obj(this); ffui_view_settextstr(this, &s); }
};

struct ffui_viewcolxx {
	ffui_viewcol vc;

	uint	width() { return ffui_viewcol_width(&vc); }
	void	width(uint val) { ffui_viewcol_setwidth(&vc, val); }
};

struct ffui_viewxx_disp : ffui_view_disp {
	uint	index() const { return ffui_view_dispinfo_index(this); }
	uint	subindex() const { return ffui_view_dispinfo_subindex(this); }

	void	text(ffstr s) { ffui_view_dispinfo_settext(this, s.ptr, s.len); }
};

struct ffui_viewxx : ffui_view {
	int		append(ffstr text) {
		ffui_viewitem vi = {};
		ffui_view_settextstr(&vi, &text);
		return ffui_view_append(this, &vi);
	}
	void	set(int idx, int col, ffstr text) {
		ffui_viewitem vi = {};
		ffui_view_setindex(&vi, idx);
		ffui_view_settextstr(&vi, &text);
		ffui_view_set(this, col, &vi);
	}
	void	update(uint first, int delta) { ffui_post_view_setdata(this, first, delta); }
	void	length(uint n, bool redraw) { ffui_view_setcount(this, n, redraw); }
	void	clear() { ffui_post_view_clear(this); }
	int		focused() { return ffui_view_focused(this); }

	ffslice	selected() { return ffui_view_selected(this); }
	int		selected_first() { return ffui_view_selected_first(this); }
	void	select(uint pos) { ffui_post_view_select_single(this, pos); }
	void	select_all() { ffui_post_view_select_single(this, ~0U); }

	ffui_viewcolxx& column(int pos, ffui_viewcolxx *vc) { ffui_view_col(this, pos, &vc->vc); return *vc; }
	void	column(uint pos, ffui_viewcolxx &vc) { ffui_view_setcol(this, pos, &vc.vc); }

	uint	scroll_vert() { return ffui_send_view_scroll(this); }
	void	scroll_vert(uint val) { ffui_post_view_scroll_set(this, val); }
	void	scroll_index(uint i) { ffui_view_scroll_idx(this, i); }

#ifdef FF_LINUX
	void	drag_drop_init(uint action_id) { ffui_view_dragdrop(this, action_id); }
#endif
};

struct ffui_windowxx : ffui_window {
	void	show(bool show) { ffui_post_wnd_show(this, show); }
	void	present() { ffui_wnd_present(this); }
	void	title(const char *sz) { ffui_send_wnd_settext(this, sz); }
	void	close() { ffui_wnd_close(this); }

	ffui_pos pos() { ffui_pos p; ffui_wnd_placement(this, &p); return p; }
	void	place(const ffui_pos &pos) { ffui_wnd_setplacement(this, SW_SHOWNORMAL, &pos); }
};

struct ffui_dialogxx : ffui_dialog {
	ffui_dialogxx() { ffui_dlg_init(this); }
	~ffui_dialogxx() { ffui_dlg_destroy(this); }
	void	title(const char *sz) { ffui_dlg_titlez(this, sz); }
	char*	save(ffui_window *parent, const char *default_name) { return ffui_dlg_save(this, parent, default_name, (default_name) ? ffsz_len(default_name) : 0); }
};

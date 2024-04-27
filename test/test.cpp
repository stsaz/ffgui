#include <ffsys/base.h>
#ifdef FF_WIN
#include <ffgui/winapi/loader.h>
#else
#include <ffgui/gtk/loader.h>
#endif
#include <ffgui/loader.h>
#include <ffgui/gui.hpp>
#include <ffsys/std.h>
#include <ffsys/globals.h>
#include <assert.h>

struct gt {
	struct wmain {
		ffui_windowxx	wnd;
	} main;

	static void* gui_ctl_find(void *udata, const ffstr *name)
	{
		#define _(m) FFUI_LDR_CTL(struct wmain, m)
		static const ffui_ldr_ctl wmain_ctls[] = {
			_(wnd),
			FFUI_LDR_CTL_END
		};
		#undef _
		static const ffui_ldr_ctl top_ctls[] = {
			FFUI_LDR_CTL3(struct gt, main, wmain_ctls),
			FFUI_LDR_CTL_END
		};
		return ffui_ldr_findctl(top_ctls, udata, name);
	}

	static int gui_cmd_find(void *udata, const ffstr *name)
	{
		return 0;
	}

	static void main_on_action(ffui_window *wnd, int id)
	{
		ffui_post_quitloop();
	}

	void load()
	{
		ffui_loader ldr;
		ffui_ldr_init(&ldr, gui_ctl_find, gui_cmd_find, this);
		if (ffui_ldr_loadfile(&ldr, "test.ui")) {
			fflog("parsing gsync.ui: %s", ffui_ldr_errstr(&ldr));
			assert(0);
		}
		ffui_ldr_fin(&ldr);

		this->main.wnd.on_action = main_on_action;
		ffui_thd_post(show, this);
	}

	static void show(void *param)
	{
		struct gt *g = (struct gt*)param;
		g->main.wnd.show(1);
	}
};
struct gt *g;

int main()
{
	g = ffmem_new(struct gt);
	ffui_init();
	g->load();
	ffui_run();
	ffmem_free(g);
	return 0;
}

/** GUI/GTK+: utils
2026, Simon Zolin */

#pragma once
#include <ffsys/file.h>
#include <ffsys/path.h>
#include <ffbase/string.h>
#include <ffbase/conf.h>

int ffui_ldr_load(ffui_loader *g, const char *window)
{
	int r, r2 = 0, if_ctx = 0;
	struct ffconf_obj c = {
		.lt.line = g->conf_line,
		.lt.line_off = g->conf_col,
	};

	ffconf_scheme cs = {};
	ffconf_scheme_addctx(&cs, top_args, g);
	g->cs = &cs;
	g->ffc = &c.lt;

	ffstr val = {};
	while (g->conf.len) {
		if (FFCONF_ERROR == (r = ffconf_obj_read(&c, &g->conf, &val)))
			goto end;
		_ffui_log("conf: %d: %S", ffconf_line(&c.lt), &val);

		if (if_ctx) {
			if (r == FFCONF_KEY && ffstr_eqcz(&val, "%ENDIF")) {
				if_ctx = 0;
				continue;
			}
			if (if_ctx < 0)
				continue;
		}
		if (r == FFCONF_KEY && ffstr_matchcz(&val, "%IF_")) {
			if (if_ctx) {
				c.lt.error = "Nested %IF blocks are not supported";
				goto end;
			}
			const char *if_name = "%IF_GTK";
#ifdef FF_WIN
			if_name = "%IF_W32";
#endif
			if_ctx = (ffstr_eq(&val, if_name, 7)) ? 1 : -1;
			continue;
		}

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

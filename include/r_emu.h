#pragma once

#include <stdbool.h>
#include <r_util.h>
#include <r_io.h>
#include <r_anal.h>
#include <r_th.h>
#include <r_emu_interactor.h>

typedef struct r_emulator_t {
	RAnal *anal;
	RIO *io;
	RList *threads;
	struct r_emulator_plugin_t *plugin;
	void *user;
} REmu;

typedef struct r_emulator_plugin_t {
	const char *name;
	const char *desc;
	const char *arch;
	void *(*init)(REmu *emu);
	//allocation
	void (*fini)(void *user);
	//deallocation
	bool (*pre_loop)(REmu *emu, RAnalOp *op);
	bool (*post_loop)(REmu *emu);
} REmuPlugin;

R_API REmuPlugin *r_emu_plugin_get(char *arch);
R_API void *r_emu_plugin_init(REmuPlugin *p, REmu *emu);
R_API void r_emu_plugin_fini(REmuPlugin *p, void *user);
R_API bool r_emu_plugin_pre_loop(REmuPlugin *p, REmu *emu, RAnalOp *op);
R_API bool r_emu_plugin_post_loop(REmuPlugin *p, REmu *emu);

R_API REmu *r_emu_new();
R_API bool r_emu_load(REmu *emu, char *arch);
R_API void r_emu_unload(REmu *emu);
R_API void r_emu_free (REmu *emu);

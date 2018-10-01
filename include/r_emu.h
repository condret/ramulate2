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
//	bool (*set_interrupts)(RAnalEsil *esil, void *user);
	//setup interrupt handlers
//	bool (*set_custom_ops)(RAnalEsil *esil, void *user);
	//setup custom esil operations
//	bool (*set_esil_hooks)(RAnalEsil *esil, void *user);
	//setup esil MODIFIER hooks for memory and register access
	bool (*pre_loop)(REmu *emu);
	bool (*post_loop)(REmu *emu);
} REmuPlugin;

#include <r_emu.h>
#include <r_io.h>
#include <r_anal.h>
#include <r_th.h>
#include <r_emu_interactor.h>
#include <gb/gb.h>
#include <stdbool.h>


void *gb_init (REmu *emu) {
	return NULL;
}

void gb_fini (void *user) {
}

bool gb_pre_loop (REmu *emu, RAnalOp *op) {
	return true;
}

bool gb_post_loop (REmu *emu) {
	return true;
}

REmuPlugin emu_gb = {
	.name = "Gameboy",
	.desc = "Emulation plugin for Gameboy",
	.arch = "gb",
	.init = gb_init,
	.fini = gb_fini,
	.pre_loop = gb_pre_loop,
	.post_loop = gb_post_loop,
};

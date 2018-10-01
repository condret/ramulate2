#pragma once

#include <stdbool.h>
#include <r_util.h>
#include <r_io.h>
#include <r_anal.h>
#include <r_th.h>
#include <r_emu.h>

#define	R_EMU_INTERACTOR_JOYPAD_A	(0x1 << 0)
#define	R_EMU_INTERACTOR_JOYPAD_B	(0x1 << 1)
#define	R_EMU_INTERACTOR_JOYPAD_START	(0x1 << 2)
#define	R_EMU_INTERACTOR_JOYPAD_SELECT	(0x1 << 3)
#define	R_EMU_INTERACTOR_JOYPAD_UP	(0x1 << 4)
#define	R_EMU_INTERACTOR_JOYPAD_DOWN	(0x1 << 5)
#define	R_EMU_INTERACTOR_JOYPAD_LEFT	(0x1 << 6)
#define	R_EMU_INTERACTOR_JOYPAD_RIGHT	(0x1 << 7)

typedef struct r_emu_interactor_t {
	struct r_emu_interactor_plugin_t *plugin;
	void *user;
	ut16 height;
	ut16 width;
	bool fullscreen;	//do we need this
	ut8 voices;
} REmuInteractor;

typedef struct r_emu_interactor_plugin_t {
	const char *name;
	const char *desc;
	void *(*init)();
	void (*fini)(REmuInteractor *ei);
	void (*init_joypad)(REmuInteractor *ei);
	ut8 (*poll_joypad)(REmuInteractor *ei);
	void (*init_screen)(REmuInteractor *ei, ut16 width, ut16 height, bool fullscreen);
	void (*set_pixel)(REmuInteractor *ei, ut16 x, ut16 y, ut32 rgb);
	void (*init_sound)(REmuInteractor *ei, ut8 voices);
//	void (*play_wave)(ut8 voice, ...)	//what to do here
} REmuInteractorPlugin;

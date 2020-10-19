#include <r_emu_interactor.h>
#include <stdbool.h>
#include <r_emu.h>
#include <r_util.h>
#include <string.h>

static REmuInteractorPlugin *interactor_plugins[]{
	NULL
};

R_API REmuInteractorPlugin *r_emu_interactor_plugin_get(char *name) {
	ut32 i = 0;

	while (interactor_plugins[i] && strcmp (interactor_plugins[i]->name, name)) {
		i++;
	}
	return interactor_plugins[i];
}

#define EIPLUG REmuInteractorPlugin

R_API void *r_emu_interactor_plugin_init(EIPLUG *eip) {
	if (!eip || !(eip->init && eip->fini)) {
		return NULL;
	}
	return eip->init ();
}

R_API void r_emu_interactor_plugin_fini(EIPLUG *eip, REmuInteractor *ei) {
	if (!eip || !eip->fini || !ei) {
		return;
	}
	eip->fini (ei);
}

R_API ut8 r_emu_interactor_plugin_poll_joypad(EIPLUG *eip, REmuInteractor *ei) {
	if (!ei || !eip || !eip->poll_joypad) {
		return 0;
	}
	return eip->poll_joypad (ei);
}

R_API void r_emu_interactor_plugin_init_screen(EIPLUG *eip, REmuInteractor *ei, ut16 width, ut16 height, bool fullscreen) {
	if (!ei || !eip || !eip->init_screen) {
		return;
	}
	eip->init_screen (ei, width, height, fullscreen);
}

R_API void r_emu_interactor_plugin_set_pixel(EIPLUG *eip, REmuInteractor *ei, ut16 x, ut16 y, ut32 rgb) {
	if (!ei || !eip || !eip->set_pixel) {
		return;
	}
	eip->set_pixel (ei, x, y, rgb);
}

R_API void r_emu_interactor_plugin_init_sound(EIPLUG *eip, REmuInteractor *ei, ut8 voices) {
	if (!ei || !eip || !eip->init_sound) {
		return;
	}
	eip->init_sound (ei, voices);
}

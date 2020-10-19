#include <r_util.h>
#include <r_emu.h>
#include <string.h>
#include <stdbool.h>

static REmuPlugin *emu_plugins[] = {
	&emu_gb,
	NULL
};

R_API REmuPlugin *r_emu_plugin_get(char *arch) {
	ut32 i = 0;

	if (!arch) {
		return NULL;
	}

	while (emu_plugins[i] && strcmp (emu_plugins[i]->arch, arch)) {
		i++;
	}
	return emu_plugins[i];
}

R_API void *r_emu_plugin_init(REmuPlugin *p, REmu *emu) {
	if (!emu || !p || !(p->init && p->fini)) {
		return NULL;
	}
	return p->init (emu);
}

R_API void r_emu_plugin_fini(REmuPlugin *p, void *user) {
	if (p && p->fini) {
		p->fini (user);
	}
}

R_API bool r_emu_plugin_pre_loop(REmuPlugin *p, REmu *emu, RAnalOp *op) {
	if (!p || !emu || !op) {
		return false;
	}
	if (!p->pre_loop) {
		return true;
	}
	return p->pre_loop (emu, op);
}

R_API bool r_emu_plugin_post_loop(REmuPlugin *p, REmu *emu) {
	if (!P || !emu) {
		return false;
	}
	if (!p->post_loop) {
		return true;
	}
	return p->post_loop (emu);
}

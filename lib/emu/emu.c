#include <r_util.h>
#include <r_io.h>
#include <r_anal.h>
#include <r_emu.h>

R_API REmu *r_emu_new() {
	REmu *emu = R_NEW0 (REmu);
	if (emu) {
		emu->io = r_io_new ();
		if (!io) {
			R_FREE (emu);
			goto breach;
		}
		emu->anal = r_anal_new ();
		if (!anal) {
			r_io_free (emu->io);
			R_FREE (emu);
		}
	}
beach:
	return emu;
}

R_API bool r_emu_load(REmu *emu, char *arch) {
	if (!arch || !emu || emu->plugin) {
		return false;
	}
	emu->plugin = r_emu_plugin_get (arch);
	if (!emu->plugin) {
		return false;
	}
	emu->user = r_emu_plugin_init (emu);
	return true;
}

R_API void r_emu_unload(REmu *emu) {
	if (emu && emu->plugin) {
		r_emu_plugin_fini (emu->user);
		emu->plugin = NULL;
		emu->user = NULL;
	}
}

R_API void r_emu_free(REmu *emu) {
	if (!emu) {
		return;
	}
	r_emu_unload (emu);
	r_io_free (emu->io);
	r_anal_free (emu->anal);
	free (emu);
}

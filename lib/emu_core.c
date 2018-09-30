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

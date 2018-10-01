#include <r_util.h>
#include <r_anal.h>
#include <r_bin.h>
#include <r_io.h>
#include <r_th.h>
#include <r_emu.h>

static int raboy (char *gb_rom_path);

int main (int argc, char *argv[]) {
	if (argc < 2) {
		eprintf ("usage: raboy gb-rom\n");
		return -1;
	}
	return raboy(argv[1]);
}

static int raboy (char *gb_rom_path) {
	RBin = r_bin_new ();
	REmu *emu;
	int fd, ret = -1;
	if (!bin) {
		eprintf ("cannot alloc r_bin\n");
		return -1;
	}

	emu = r_emu_new();
	if (!emu) {
		eprintf ("cannot alloc r_emu\n");
		r_bin_free (bin);
		return -1;
	}

	fd = r_io_fd_open (emu->io, gb_rom_path, R_IO_READ, 0644);
	r_bin_iobind (bin, io);
	if (!r_bin_load_io (bin, fd, 0, 0, 0, 0, NULL)) {
		eprintf ("not a gb rom\n");
		r_bin_free (bin);
		goto beach;
	}
	RBinInfo *info = r_bin_get_info (bin);
	if (!info || strcmp (info->arch, "gb")) {
		eprintf ("not a gb rom\n");
		r_bin_free (bin);
		goto beach;
	}
	r_bin_free (bin);

//init allegro
//init gb
//enter loop
//fini gb
//fini allegro

beach:
	r_emu_free (emu);
	return ret;


#include <r_emu.h>
#include <r_io.h>
#include <r_anal.h>
#include <r_reg.h>
#include <r_th.h>
#include <r_util.h>
#include <r_emu_interactor.h>
#include <gb/gb.h>
#include <stdbool.h>

#ifndef	R_IO_RWX
#define	R_IO_RWX	R_IO_RW | R_IO_EXEC
#endif
#ifndef	R_IO_RX
#define	R_IO_RX		R_IO_READ | R_IO_EXEC
#endif

#if 0
char gbstrbuf[128];

static RIODesc *__gb_iflags_open (RIO *io, const char *path, int rwx, int mode) {
	RIODesc *desc;

	desc = R_NEW0 (RIODesc);
	if (desc) {
		desc->data = (void *)(size_t)r_num_get (NULL, &path[12]);
		desc->flags = rwx;
	}
	return desc;
}

static int __gb_iflags_close (RIODesc *desc) {
	return 0;
}

static int __gb_iflags_read (RIO *io, RIODesc *desc, ut8 *buf, int len) {
	if (desc && desc->data && buf && len) {
		buf[0] = ((ut8 *)(desc->data))[0];
		return 1;
	}
	return 0;
}

static int __gb_iflags_write (RIO *io, RIODesc *desc, ut8 *buf, int len) {
	if (desc && desc->data && buf && len) {
		((ut8 *)(desc->data))[0] = buf[0];
		return 1;
	}
	reutrn 0;
}

static ut64 __gb_iflags_lseek (RIO *io, RIODesc *desc, ut64 off, int whence) {
	return (whence == R_IO_SEEK_END) ? 1 : 
		(whence == R_IO_SEEK_SET) ? (off > 0) : 0;	//XXX
}

static bool __gb_iflags_check (RIO *io, const char *path, bool many) {
	return path && (strstr(path, "gb_iflags://") == path);
}

RIOPlugin r_io_wild_gb_iflags_plugin {
	.name = "gb_interrupt_flags",
	.desc = "Show status of the pending interrupts",
	.license = "LGPL3",
	.open = __gb_iflags_open,
	.close = __gb_iflags_close,
	.read = __gb_iflags_read,
	.write = __gb_iflags_write,
	.lseek = __gb_iflags_lseek,
	.check = __gb_iflags_check,
};
#endif

static void *gb_init (REmu *emu) {
	Gameboy *gb = R_NEW(Gameboy);
	if (!gb) {
		return NULL;
	}
	gb->hram_fd = r_io_fd_open (emu->io, "malloc://0x7f", R_IO_RWX, 0644);
	gb->hram_map_id = r_map_add (emu->io, gb->hram_fd, R_IO_RWX, 0LL, 0xff80, 0x7f);
	gb->vram_fd = r_io_fd_open (emu->io, "malloc://0x2000", R_IO_RWX, 0644);
	gb->vram_map_id = r_io_map_add (emu->io, gb->vram_fd, R_IO_RWX, 0LL, 0x8000, 0x2000);
#if 0
	sprintf (gbstrbuf, "gb_iflags://%p", &gb->interrupt_flags);
	gb->iflags_fd = r_io_desc_open_plugin(emu->io, &r_io_wild_gb_iflags_plugin, gbstrbuf, R_IO_RWX, 0644)->fd;	//well, this might segfault, but E_TOO_LAZY
	r_io_map_add (emu->io, gb->iflags_fd, R_IO_RWX, 0LL, 0xff0f, 1);
#endif
	gb->if_fd = r_io_fd_open (emu->io, "malloc://1", R_IO_RWX, 0644);
	r_io_map_add (emu->io, gb->if_fd, R_IO_RWX, 0LL, 0xff0f, 1);
	gb->ie_fd = r_io_fd_open (emu->io, "malloc://1", R_IO_RWX, 0644);
	r_io_map_add (emu->io, gb->ie_fd, R_IO_RWX, 0LL, 0xffff, 1);

	return gb;
}

static void gb_fini (void *user) {
	free (user);	//io will cleanup itself :D
}

static bool gb_pre_loop (REmu *emu, RAnalOp *op, ut8 *bytes) {
	Gameboy *gb = (Gameboy *)emu->user;
	if (bytes[0] == 0x10) {
		r_strbuf_set (op->esil, "STOP");
		gb->sleep = op->cycles = 0;
		return true;
	}
	if (bytes[0] == 0x76) {
		r_strbuf_set (op->esil, "HALT");
		gb->sleep = op->cycles = 0;
		return true;
	}

	switch (op->type) {
	case R_ANAL_OP_TYPE_CRET:	//I'm a condret :)
	case R_ANAL_OP_TYPE_CJMP:
	case R_ANAL_OP_TYPE_UCJMP:
	case R_ANAL_OP_TYPE_CCALL:
	case R_ANAL_OP_TYPE_UCCALL:
		gb->not_match_sleep_addr = op->addr + op->size;
		gb->not_match_sleep = op->cycles - op->failcycles;
	}
	if (op->failcycles)
	return true;
}

static bool gb_post_loop (REmu *emu) {
	Gameboy *gb;
	ut8 ime, iflags, ieflags, joypad, iv, intr, i;
	if (!emu || !emu->user) {
		return false;
	}
	ime = r_reg_getv(emu->anal->reg, "ime");
	r_io_fd_read_at (emu->io, gb->if_fd, 0LL, &iflags, 1);
	r_io_fd_read_at (emu->io, gb->ie_fd, 0LL, &ieflags, 1);
	joypad = r_emu_interactor_poll_joypad (emu->interactor);
	if (joypad != gb->joypad) {
		iflags |= 0x10;
	}
	iv = ime ? iflags & eiflags : 0;
	intr = 0;
	for (i = 0; i < 5; i++) {
		if (iv & (1<<i)) {
			intr = 40 + i << 3;
			break;
		}
	}
	if (intr) {
		r_anal_esil_fire_interrupt (emu->esil, intr);
	}
	//TODO
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

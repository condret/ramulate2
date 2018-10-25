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


typedef struct gameboy_seek_t {
	Gameboy *gb;
	ut64 off;
} GBSeek;

char gbstrbuf[128];

static RIODesc *__gb_timers_open (RIO *io, const char *path, int rwx, int mode) {
	RIODesc *desc;
	GBSeek *gbseek = R_NEW0(GBSeek);
	if (!gbseek) {
		return NULL;
	}

	desc = R_NEW0 (RIODesc);
	if (!desc) {
		free (gbseek);
		return NULL;
	}

	gbseek->gb = (Gameboy *)(size_t)r_num_get (NULL, &path[12]);
	desc->data = gbseek;
	desc->flags = rwx;

	return desc;
}

static int __gb_timers_close (RIODesc *desc) {
	if (desc) {
		free (desc->data);
	}
	return 0;
}

static int __gb_timers_read (RIO *io, RIODesc *desc, ut8 *buf, int len) {
	GBSeek *gbs;
	ut32 elen, ret;		//length to read
	if (!(desc && desc->data && buf && len)) {
		return 0;
	}
	gbs = (GBSeek *)desc->data;
	if (gbs->off > 3) {	//out of file
		return 0;
	}
	elen = R_MIN (len, 4) - gbs->off;
	for (ret = 0; ret < elen; ret++) {
		switch (gbs->off) {
		case 0:		//div
			buf[ret] = gb_get_div(gbs->gb->timers);
			break;
		case 1:		//tima
			buf[ret] = gb_get_tima(gbs->gb->timers);
			break;
		case 2:		//tma
			buf[ret] = gbs->gb->timers.tma;
			break;
		case 3:		//tac
			buf[ret] = gbs->gb->timers.tac;
			break;
		}
		gbs->off++;
	}

	return ret;
}

static int __gb_timers_write (RIO *io, RIODesc *desc, ut8 *buf, int len) {
	GBSeek *gbs;
	ut32 elen, ret;		//length to read
	if (!(desc && desc->data && buf && len)) {
		return 0;
	}
	gbs = (GBSeek *)desc->data;
	if (gbs->off > 3) {	//out of file
		return 0;
	}
	elen = R_MIN (len, 4) - gbs->off;
	for (ret = 0; ret < elen; ret++) {
		switch (gbs->off) {
		case 0:		//div
			gbs->gb->timers.div = 0;
			break;
		case 3:		//tac
			gbs->gb->timers.tac = buf[ret] & 7;
		case 1:		//tima
			gbs->gb->timers.tima = 0;	//is this correct?
			break;
		case 2:		//tma
			gbs->gb->timers.tma = buf[ret];
			break;
		}
		gbs->off++;
	}

	return ret;
}

static ut64 __gb_timers_lseek (RIO *io, RIODesc *desc, ut64 off, int whence) {
	GBSeek *gbs;
	if (!(desc && desc->data)) {
		return 0LL;
	}
	gbs = (GBSeek *)desc->data;
	switch (whence) {
	case R_IO_SEEK_END:
		gbs->off = 4 + off;
		break;
	case R_IO_SEEK_SET:
		gbs->off = off;
		break;
	case R_IO_SEEK_CUR:
		gbs->off += off;
		break;
	}
	return gbs->off;
}

static bool __gb_timers_check (RIO *io, const char *path, bool many) {
	return path && (strstr(path, "gb_timers://") == path);
}

RIOPlugin r_io_wild_gb_timers_plugin {
	.name = "gb_timers",
	.desc = "Represent GB-Timers",
	.license = "LGPL3",
	.open = __gb_timers_open,
	.close = __gb_timers_close,
	.read = __gb_timers_read,
	.write = __gb_timers_write,
	.lseek = __gb_timers_lseek,
	.check = __gb_timers_check,
};

static void gb_lock_oam(Gameboy *gb, RIO *io) {
	RIOMap *oam_map;
	gb->oam_lock++;
	if (gb->oam_lock == 1) {
		return;
	}

	//this locks oam access from r_io_read_at and r_io_write_at
	oam_map = r_io_map_resolve (io, gb->oam_map_id);
	oam_map->flags = 0;
}

static void gb_unlock_oam(Gameboy *gb, RIO *io) {
	RIOMap *oam_map;
	switch (gb->oam_lock) {
	case 0:
		break;
	case 1:
		oam_map = r_io_map_resolve (io, gb->oam_map_id);
		oam_map->flags = R_IO_RWX;
	default:
		gb->oam_lock--;
	}
}

static void gb_enter_dma(Gameboy *gb, RIO *io) {
	if (gb->screen.dma.remaining_cycles) {	//not sure if reentering is possible
		return;
	}

	gb->screen.dma.remaining_cycles = GB_DMA_COPY_CPU_CYCLES;
	gb->screen.dma.src = gb->screen.dma.reg << 8;
	gb_lock_oam (gb, io);
}

static void gb_leave_dma(Gameboy *gb, RIO *io) {
	if (gb->screen.dma.remaining_cycles) {	//not finished
		return;
	}

	gb->screen.dma.src = 0;
	gb_unlock_oam (gb, io);
}

static void gb_proceed_dma(Gameboy *gb, RIO *io, ut32 cycles) {
	ut8 buf[16];
	ut64 src, dst;
	ut32 len;
	if (!gb->screen.dma.remaining_cycles) {
		return;
	}
	dst = (GB_DMA_COPY_CPU_CYCLES - gb->screen.dma.remaining_cycles) >> 2;
	gb->screen.dma.remaining_cycles -= R_MIN (gb->screen.dma.remaining_cycles, cycles);
	len = ((GB_DMA_COPY_CPU_CYCLES - gb->screen.dma.remaining_cycles) >> 2) - dst;
	src = gb->screen.dma.src + dst;
	r_io_read_at (io, src, buf, len);
	r_io_fd_write_at (io, gb->oam_fd, buf, len);
	if (!gb->screen.dma.remaining_cycles) {
		gb_leave_dma (gb, io);
	}
}

static void gb_enter_oam_search(Gameboy *gb, RIO *io) {
	ut8 mode = gb->screen.stat & GB_LCD_STAT_MODE_MASK;
	if ((mode != GB_LCD_STAT_MODE_VBLANK) && (mode != GB_LCD_STAT_MODE_HBLANK)) {	//check if entering from vblank or hblank
		return;
	}
	gb->ppu.remaining_cycles = GB_OAM_SEARCH_CPU_CYCLES;
	gb->ppu.idx = 0;
	gb->screen.stat &= ~GB_LCD_STAT_MODE_MASK;
	gb->screen.stat |= GB_LCD_STAT_MODE_OAM_SEARCH;
	gb_lock_oam (gb, io);
}

static void gb_leave_oam_search(Gameboy *gb, RIO *io) {
	if ((gb->screen.stat & GB_LCD_STAT_MODE_MASK) == GB_LCD_STAT_MODE_OAM_SEARCH) {	//check if leaving oam-search
		gb_unlock_oam (gb, io);
	}
}

//read 160 bytes in 80 cpu-cycles -> 2 bytes per cycle
static void gb_proceed_oam_search(Gameboy *gb, RIO *io, ut32 cycles) {
	ut64 off;
	ut16 *in;
	if (!gb->ppu.remaining_cycles) {
		return;
	}
	cycles = R_MIN (gb->ppu.remaining_cycles, cycles);
	in = (ut16 *)gb->ppu.sprites;
	off = (GB_OAM_SEARCH_CPU_CYCLES - gb->ppu->remaining_cycles) << 1;
	const ut8 height = 8 + ((gb->screen.lcdc & 4) << 1);
	while (cycles) {
		if (gb->ppu.idx == 20) {
			break;
		}
		r_io_fd_read_at (io, gb->oam_fd, off, &in[gb->ppu.idx], 2);
		gb->ppu.idx++;
		if (!(gb->ppu.idx & 1)) {
			const s = (gb->ppu.idx >> 1) - 1;
			//check if sprite is in the line
			if (!((gb->ppu.sprites[s].x) &&
				(gb->ppu.sprites[s].y <= (gb->screen.ly + 16)) &&
				((gb->ppu.sprites[s].y + height) > gb->screen.ly))) {
				gb->ppu.idx = s << 1;	//sprite is not relevant, so skip it
			}
		}
		off += 2;
		cycles--;
		gb->ppu.remaining_cycles--;
	}
	if (!(gb->ppu.remaining_cycles -= cycles)) {
		gb->ppu.idx >>= 1;
		gb_leave_oam_search(gb, io);
	}
}

static GBOamEntry *gb_get_next_oam_ptr (Gameboy *gb, ut32 *ctr, ut16 y) {
	while (ctr[0] < gb->ppu.idx) {
		const idx = ctr[0];
		ctr[0]++;
		if (gb->ppu.sprites[idx].y == y) {
			return &gb->ppu.sprites[idx];
		}
	}
	return NULL;
}

#if 0
static void gb_proceed_dma(Gameboy *gb, RIO *io, ut32 cycles) {
	ut8 buf[16];
	ut32 ccl, len;
	ut64 src, dst;
	if (!gb->screen.dma.remaining_cycles) {
		return;
	}
	// rethink this
	ccl = cycles | gb->screen.dma.mod;
	gb->screen.dma.mod = ccl & 3;	// mod 4 bc we use cpu cycles
	ccl &= ~3;
	if (ccl > gb->screen.dma.remaining_cycles) {
		ccl = gb->screen.dma.remaining_cycles;
	}
	dst = (640 - gb->screen.dma.remaining_cycles) >> 2;
	src = gb->screen.dma.src + dst;
	len = ccl >> 2;
	r_io_read_at (io, src, buf, len);
	r_io_fd_write_at (io, gb->oam_fd, dst, buf, len);
	gb->screen.dma.remaining_cycles -= ccl;
	if (!gb->screen.dma.remaining_cycles) {
		gb_leave_dma (gb, io);
	}
}
#endif

static void gb_lock_vram(Gameboy *gb, RIO *io) {
	RIOMap *vram_map = r_io_map_resolve(io, gb->vram_map_id);
	vram_map->flags = 0;
}

static void gb_unlock_vram(Gameboy *gb, RIO *io) {
	RIOMap *vram_map = r_io_map_resolve(io, gb->vram_map_id);
	vram_map->flags = R_IO_RWX;
}

static ut32 gb_mix_8_pixels(ut32 in, ut32 mix_in) {
	ut32 i, mix = 0;
	ut8 pixel;

	for (i = 0; i < 8; i++) {
		mix >>= 4;
//	|ss|cc|ss|cc|ss|cc|ss|cc|ss|cc|ss|cc|ss|cc|ss|cc|
// Fix this
		pixel = in & 0x0f;
		if (!(pixel & 0x0c)) {		//check if pixel is not a sprite
			if (mix_in & 0x03) {	//check if mix_in_pixel is not background
				pixel = mix_in & 0x0f;
			}
		}
		mix |= pixel << 28;
		in >>= 4;
		mix_in >>= 4;
	}
	return mix;
}

static int __gb_screen_read (RIO *io, RIODesc *desc, ut8 *buf, int len) {
	GBSeek *gbs;
	ut32 elen, ret;		//length to read
	if (!(desc && desc->data && buf && len)) {
		return 0;
	}
	gbs = (GBSeek *)desc->data;
	if (gbs->off > 11) {	//out of file
		return 0;
	}
	elen = R_MIN (len, 12) - gbs->off;
	for (ret = 0; ret < elen; ret++) {
		switch (gbs->off) {
		case 0x00:		//lcd controll
			buf[ret] = gbs->gb->screen.lcdc;
			break;.
		case 0x01:		//lcd status
			buf[ret] = gbs->gb->screen.stat;
			break;
		case 0x02:		//scy
			buf[ret] = gbs->gb->screen.scy;
			break;
		case 0x03:
			buf[ret] = gbs->gb->screen.scx;
			break;
		case 0x04:
			buf[ret] = gbs->gb->screen.ly;
			break;
		case 0x05:
			buf[ret] = gbs->gb->screen.lyc;
			break;
		case 0x06:		//pandocs lie about this, this is not write-only
			buf[ret] = gbs->gb->screen.dma.reg;
			break;
		case 0x07:		//background palette
			buf[ret] = gbs->gb->screen.bgp;
			break;
		case 0x08:		//object palette 0
			buf[ret] = gbs->gb->screen.obp0;
			break;
		case 0x09:		//object palette 1
			buf[ret] = gbs->gb->screen.obp1;
			break;
		case 0x0a:		//window y-position
			buf[ret] = gbs.gb->screen.wy;
			break;
		case 0x0b:		//window x-position
			buf[ret] = gbs.gb->screen.wx;
			break;
			//TODO
		}
		gbs->off++;
	}

	return ret;
}

static int __gb_screen_write (RIO *io, RIODesc *desc, ut8 *buf, int len) {
	GBSeek *gbs;
	ut32 elen, ret;		//length to read
	if (!(desc && desc->data && buf && len)) {
		return 0;
	}
	gbs = (GBSeek *)desc->data;
	if (gbs->off > 11) {	//out of file
		return 0;
	}
	elen = R_MIN (len, 12) - gbs->off;
	for (ret = 0; ret < elen; ret++) {
		switch (gbs->off) {
		case 0x00:		//lcd control
			gbs->gb->screen.lcdc = buf[ret];
			break;
		case 0x01:		//lcd status
			gbs->gb->screen.stat = buf[ret] & 0xfc;
			// bit 0 and 1 are read only
			break;
		case 0x02:		//scy
			gbs->gb->screen.scy = bur[ret];
			break;
		case 0x03:
			gbs->gb->screen.scx = bur[ret];
			break;
		case 0x05:
			gbs->gb->screen.lyc = bur[ret];
			break;
		case 0x06:		//dma
			gbs->gb->screen.dma.reg = buf[ret];
			gbs->gb_enter_dma (gbs->gb, io);
			break;
		case 0x07:		//background palette
			gbs->gb->screen.bgp = buf[ret];
			break;
		case 0x08:		//object palette 0
			gbs->gb->screen.obp0 = buf[ret];
			break;
		case 0x09:		//object palette 1
			gbs->gb->screen.obp1 = buf[ret]; 
			break;
		case 0x0a:		//window y-position
			gbs.gb->screen.wy = buf[ret];
			break;
		case 0x0b:		//window x-position
			gbs.gb->screen.wx = buf[ret];
			break;
			//TODO
		}
		gbs->off++;
	}

	return ret;
}

static ut64 __gb_screen_lseek (RIO *io, RIODesc *desc, ut64 off, int whence) {
	GBSeek *gbs;
	if (!(desc && desc->data)) {
		return 0LL;
	}
	gbs = (GBSeek *)desc->data;
	switch (whence) {
	case R_IO_SEEK_END:
		gbs->off = 12 + off;
		break;
	case R_IO_SEEK_SET:
		gbs->off = off;
		break;
	case R_IO_SEEK_CUR:
		gbs->off += off;
		break;
	}
	return gbs->off;
}

static bool __gb_screen_check (RIO *io, const char *path, bool many) {
	return path && (strstr(path, "gb_screen://") == path);
}

RIOPlugin r_io_wild_gb_screen_plugin {
	.name = "gb_screen",
	.desc = "Represent GB-Timers",
	.license = "LGPL3",
	.open = __gb_timers_open,	//it works in the exact same way
	.close = __gb_timers_close,	//no need for duplicated functions
	.read = __gb_screen_read,
	.write = __gb_screen_write,
	.lseek = __gb_screen_lseek,
	.check = __gb_screen_check,
};

static void *gb_init (REmu *emu) {
	Gameboy *gb = R_NEW(Gameboy);
	if (!gb) {
		return NULL;
	}
	gb->hram_fd = r_io_fd_open (emu->io, "malloc://0x7f", R_IO_RWX, 0644);
	gb->hram_map_id = r_map_add (emu->io, gb->hram_fd, R_IO_RWX, 0LL, 0xff80, 0x7f);
	gb->vram_fd = r_io_fd_open (emu->io, "malloc://0x2000", R_IO_RWX, 0644);
	gb->vram_map_id = r_io_map_add (emu->io, gb->vram_fd, R_IO_RWX, 0LL, 0x8000, 0x2000);
	gb->oam_fd = r_io_fd_open (emu->io, "malloc://0xa0", R_IO_RWX, 0644);
	gb->oam_map_id = r_io_map_add (emu->io, gb->oam_fd, R_IO_RWX, 0LL, 0xfe00);
	sprintf (gbstrbuf, "gb_timers://%p", gb);
	gb->timers_fd = r_io_desc_open_plugin(emu->io, &r_io_wild_gb_timers_plugin, gbstrbuf, R_IO_RWX, 0644)->fd;	//well, this might segfault, but E_TOO_LAZY
	gb->timers_map_id = r_io_map_add (emu->io, gb->timers_fd, R_IO_RWX, 0LL, 0xff04, 4);
	sprintf (gbstrbuf, "gb_screen://%p", gb);
	gb->screen_fd = r_io_desc_open_plugin(emu->io, &r_io_wild_gb_screen_plugin, gbstrbuf, R_IO_RWX, 0644)->fd;
	gb->screen_map_id = r_io_map_add (emu->io, gb->screen_fd, R_IO_RWX, 0LL, 0xff40, 12);
	gb->if_fd = r_io_fd_open (emu->io, "malloc://1", R_IO_RWX, 0644);
	r_io_map_add (emu->io, gb->if_fd, R_IO_RWX, 0LL, 0xff0f, 1);
	gb->ie_fd = r_io_fd_open (emu->io, "malloc://1", R_IO_RWX, 0644);
	r_io_map_add (emu->io, gb->ie_fd, R_IO_RWX, 0LL, 0xffff, 1);

	gb->screen.dma.reg = 0xff;

	return gb;
}

static void gb_fini (void *user) {
	free (user);	//io will cleanup itself :D
}

static void gb_proceed_tima (Gameboy *gb, ut32 cycles) {
	if (!(gb->timers.tac & 0x4)) {
		return;
	}
	switch (gb->timers.tac & 0x3) {
	case 0:
		cycles >>= 2;
		break;
	case 1:
		cycles <<= 4;
		break;
	case 2:
		cycles <<= 2;
		break;
	}
	gb->timers.tima += cycles;
	if (gb->timers.tima & 0xffff0000) {
		gb->timers.tima = 0xff | (gb->timers.tma << 8);
		//gb_request_interrupt(gb, 2);
	}
}

static bool gb_pre_loop (REmu *emu, RAnalOp *op, ut8 *bytes) {
	Gameboy *gb = (Gameboy *)emu->user;
	if (bytes[0] == 0x10) {
		r_strbuf_set (op->esil, "STOP");
		gb->sleep = op->cycles = 0;
		//wait for sleeper to wake up
		return true;
	}
	if (bytes[0] == 0x76) {
		r_strbuf_set (op->esil, "HALT");
		gb->sleep = op->cycles = 0;
		//wait for sleeper to wake up
		return true;
	}

	//prepare sleeper for next instruction
	r_emu_th_lock_lock (&gb->sleeper->lock)
	gb->sleeper->to_sleep = op->cycles;
	//wait for sleeper to wake up
	while (!gb->sleeper->sleeping) {
		r_emu_nop();
	}
	//activate sleeper here:
	r_emu_th_lock_unlock (&gb->sleeper->lock)

	gb_proceed_div (gb->timers, op->cycles);
	gb_proceed_tima (gb, op->cycles);
	gb_proceed_dma (gb, emu->io, op->cycles);

	switch (op->type) {
	case R_ANAL_OP_TYPE_CRET:	//I'm a condret :)
	case R_ANAL_OP_TYPE_CJMP:
	case R_ANAL_OP_TYPE_UCJMP:
	case R_ANAL_OP_TYPE_CCALL:
	case R_ANAL_OP_TYPE_UCCALL:
		gb->not_match_sleep_addr = op->addr + op->size;
		gb->not_match_sleep = op->cycles - op->failcycles;
	}
	return true;
}

static bool gb_post_loop (REmu *emu) {
	Gameboy *gb;
	ut8 ime, iflags, ieflags, joypad, iv, intr, i;
	ut16 pc;
	if (!emu || !emu->user) {
		return false;
	}
	gb = (Gameboy *)emu->user;
	pc = r_reg_getv(emu->anal->reg, "pc");
	if (pc != gb->not_match_sleep_addr) {
		r_emu_th_lock_lock (&gb->sleeper->lock);
		gb->sleeper->to_sleep += gb->not_match_sleep;
		r_emu_th_lock_unlock (&gb->sleeper->lock);
		gb_proceed_div (gb->timers, gb->not_match_sleep);
		gb_proceed_tima (gb, gb->not_match_sleep);
		gb_proceed_dma (gb, emu->io, gb->not_match_sleep);
	}
	gb->sleep = 0;
	gb->not_match_sleep = 0;
	gb->not_match_sleep_addr = 0LL;

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

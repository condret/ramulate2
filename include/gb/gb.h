#pragma once

#include <r_util.h>
#include <r_th.h>
#include <r_io.h>

//pass this to an io-plugin, then map it to 0xff00
//each register needs a set-fcn in the io-plugin which can trigger certain events
typedef struct gameboy_memory_mapped_serial_transfere_t {
	ut8 sb;		//0000 : ff01	
	ut8 sc;		//0001 : ff02
} GBMMST;

typedef struct gameboy_memory_mapped_timers_t {
	ut16 div;	//0000 : ff04	// only upper byte gets exposed
	ut32 tima;	//0001 : ff05	// only &0xff00 >> 8 gets exposed
	ut8 tma;	//0002 : ff06
	ut8 tac;	//0003 : ff07
} GBMMTMR;

#define	gb_proceed_div(timers, cycles)	((timers).div += (cycles))
#define	gb_set_div(timers, val)	((timers).div = ((val) & 0xff) << 8)
#define	gb_get_div(timers)	(((timers).div & 0xff00) >> 8)
#define	gb_get_tima(timers)	(((timers).tima & 0xff0000) >> 16)

typedef struct gameboy_memory_mapped_sound_t {
	ut8 nr[23];	//0000 : ff10 - ff26 //closed interval
// in plugin : ffffffffffffffff...
	ut8 wav[16];	//0020 : ff30 - ff3f
} GBMMSND;

typedef struct gameboy_memory_mapped_dma_t {
	ut8 reg;
	ut16 src;
//	ut8 mod;
	ut16 remaining_cycles;
} GBDMA;

typedef struct gameboy_memory_mapped_screen_t {
	ut8 lcdc;	//ff40
	ut8 stat;	//ff41
	ut8 scy;	//ff42
	ut8 scx;	//ff43
	ut8 ly;		//ff44
	ut8 lyc;	//ff45
	GBDMA dma;	//ff46
	ut8 bgp;	//ff47
	ut8 obp0;	//ff48
	ut8 obp1;	//ff49
	ut8 wy;		//ff4a
	ut8 wx;		//ff4b
} GBMMSCR;

typedef struct gameboy_sleeper_t {
	R_EMU_TH_TID tid;
	R_EMU_TH_LOCK lock;
	bool repeat;
	bool sleeping;
	ut32 to_sleep;		//unit: 100ns, bc windows cannot handle the truth
} GBSleeper;

typedef struct gameboy_t {
	ut32 sleep;
	ut32 not_match_sleep;
	ut64 not_match_sleep_addr;
	GBSleeper *sleeper;
	int hram_fd;		//this is r_io_fd_open (io, "malloc://0x7f", R_IO_RW | R_IO_EXEC, 0644);
	ut32 hram_map_id;	//this is r_io_map_add (io, hram_fd, R_IO_RW | R_IO_EXEC, 0LL, 0xff80, 0x7f);
	int vram_fd;		//this is r_io_fd_open (io, "malloc://0x2000", R_IO_RW | R_IO_EXEC, 0644);
	ut32 vram_map_id;	//this is r_io_map_add (io, vram_fd, R_IO_RW | R_IO_EXEC, 0LL, 0x8000, 0x2000)->id;
//for vblank and hblank we can just set the map->flags to 0 (use the API, don't d0 it by hand).
//for dm access, via dma register, we can either temporary enable access via highlevel r_io_read_at
//or we use the r_io_fd api, since the io-fd is still accessible, when the map is not
	int oam_fd;
	ut32 oam_map_id;
	int timers_fd;
	ut32 timers_map_id;
	int screen_fd;
	ut32 screen_map_id;
	int if_fd;
	int ie_fd;
/*	-----------	*/
	ut8 joypad;		//ff00
	GBMMST serial;
	GBMMTMR timers;
	GBMMSCR screen;
	ut8 interrupt_enable;	//ffff
} Gameboy;


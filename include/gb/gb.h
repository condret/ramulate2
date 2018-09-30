#pragma once

#include <r_util.h>
#include <r_th.h>
#include <r_io.h>

//pass this to an io-plugin, then map it to 0xff00
//each register needs a set-fcn in the io-plugin which can trigger certain events
typedef struct gameboy_memory_mapped_io_t {
	ut8 p1;		//ff00
	ut8 sb;		//ff01
	ut8 sc;		//ff02
	ut8 div;	//ff04
	ut8 tima;	//ff05
	ut8 tma;	//ff06
	ut8 tac;	//ff07
/*	--------	*/
	ut8 _if;	//ff0f
/*	--------	*/
	ut8 nr[23];	//ff10 - ff26 //closed interval
	ut8 wav[16];	//ff30 - ff3f
/*	--------	*/
	ut8 lcdc;	//ff40
	ut8 stat;	//ff41
	ut8 scy;	//ff42
	ut8 scx;	//ff43
	ut8 ly;		//ff44
	ut8 lyc;	//ff45
	ut8 dma;	//ff46
	ut8 bgp;	//ff47
	ut8 obp0;	//ff48
	ut8 obp1;	//ff49
	ut8 wy;		//ff4a
	ut8 wx;		//ff4b
} GBMMIO;

typedef struct gameboy_t {
	GBMMIO mmio;
} Gameboy;


#pragma once

#include <r_util.h>
#include <r_io.h>
#include <r_anal.h>
#include <r_th.h>
#include <allegro.h>
#include <gb/gb.h>

typedef struct r_emulator_t {
	RAnal *anal;
	RIO *io;
	RList *threads;
	BITMAP *screen;		//allegro
	GameBoy *gb;		//gameboy specific
} REmu;

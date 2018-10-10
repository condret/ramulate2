#pragma once

#include <stdbool.h>
#include <r_util.h>
#include <r_io.h>
#include <r_anal.h>
#include <r_emu_interactor.h>

typedef struct r_emulator_t {
	RAnal *anal;
	RIO *io;
	RList *threads;
	struct r_emulator_plugin_t *plugin;
	void *user;
} REmu;

typedef struct r_emulator_plugin_t {
	const char *name;
	const char *desc;
	const char *arch;
	void *(*init)(REmu *emu);
	//allocation
	void (*fini)(void *user);
	//deallocation
	bool (*pre_loop)(REmu *emu, RAnalOp *op);
	bool (*post_loop)(REmu *emu);
} REmuPlugin;

R_API REmuPlugin *r_emu_plugin_get(char *arch);
R_API void *r_emu_plugin_init(REmuPlugin *p, REmu *emu);
R_API void r_emu_plugin_fini(REmuPlugin *p, void *user);
R_API bool r_emu_plugin_pre_loop(REmuPlugin *p, REmu *emu, RAnalOp *op);
R_API bool r_emu_plugin_post_loop(REmuPlugin *p, REmu *emu);

R_API REmu *r_emu_new();
R_API bool r_emu_load(REmu *emu, char *arch);
R_API void r_emu_unload(REmu *emu);
R_API void r_emu_free (REmu *emu);


#define	r_emu_nop()	asm volatile ("nop")

#if __WINDOWS__
#include <windows.h>
#define	R_EMU_TH_TID	HANDLE
/*	disgusting comma abuse hack below	*/
/* this code make CriticalSection feel a bit like pthread_mutex, if nothing breaks */
#define	r_emu_th_tid_create(x,y,z)	(((x)=CreateThread(NULL, 0, y, z, 0, NULL)), 0)
#define	r_emu_th_tid_join(x)	WaitForSingleObject(x,INFINITE)
#define	R_EMU_TH_LOCK	CRITICAL_SECTION
#define	r_emu_th_lock_init(x)	(InitializeCriticalSection(x), 0)
#define	r_emu_th_lock_fini(x)	(DeleteCriticalSection(x), 0)
#define	r_emu_th_lock_lock(x)	(EnterCriticalSection(x), 0)
#define	r_emu_th_lock_unlock(x)	(LeaveCriticalSection(x), 0)
#else
#include <pthread.h>
#define	R_EMU_TH_TID	pthread_t
#define	r_emu_th_tid_create(x,y,z)	pthread_create(x, NULL, y, z)
#define	r_emu_th_tid_join(x)	pthread_join(x, NULL)
#define	R_EMU_TH_LOCK	pthread_mutex_t
#define	r_emu_th_lock_init(x)	pthread_mutex_init(x, PTHREAD_MUTEX_NORMAL)
#define	r_emu_th_lock_fini(x)	pthread_mutex_destroy(x)
#define	r_emu_th_lock_lock(x)	pthread_mutex_lock(x)
#define	r_emu_th_lock_unlock(x)	pthread_mutex_unlock(x)
#endif

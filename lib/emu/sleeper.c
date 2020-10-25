#include <time.h>
#include <math.h>
#include <r_emu.h>
#include <r_th.h>
#include <r_types.h>

// compile with -lm

RThreadFunctionRet sleeper_fnc (RThread *th) {
	REmuSleeper *sleeper = (REmuSleeper *)th->user;
	double last_remaining = 0;
	while (!th->breaked) {
		while (!sleeper->cycles) {
			if (th->breaked) {
				return R_TH_STOP;
			}
			r_emu_nop();
		}
		r_th_lock_enter (sleeper->lock);
		const double ns_sleep_timed = sleeper->cycle_duration * sleeper->cycles + last_remaining;
		sleeper->cycles = 0;
		last_remaining = ns_sleep_timed - floor (ns_sleep_timed);
		const ut64 ns_sleep_time = floor (ns_sleep_timed);
		nanosleep ((const struct timespec[]){ { 0, ns_sleep_time } }, NULL);
		r_th_lock_leave (sleeper->lock);
	}
	return R_TH_STOP;
}

R_API REmuSleeper *r_emu_sleeper_new (double cycle_duration) {
	REmuSleeper *sleeper = R_NEW0 (REmuSleeper);
	if (!sleeper) {
		return NULL;
	}
	sleeper->lock = r_th_lock_new (true);
	if (!sleeper->lock) {
		free (sleeper);
		return NULL;
	}
	sleeper->me = r_th_new (sleeper_fnc, sleeper, 0);
	if (!sleeper->me) {
		r_th_lock_free (sleeper->lock);
		free (sleeper);
		return NULL;
	}
	sleeper->cycle_duration = cycle_duration;
	r_th_start (sleeper->me, true);
	return sleeper;
}

R_API void r_emu_sleeper_wait_for_wakeup_and_set_cycles (REmuSleeper *sleeper, ut32 cycles) {
	if (!sleeper) {
		return;
	}
	r_th_lock_enter (sleeper->lock);
	sleeper->cycles = cycles;
	r_th_lock_leave (sleeper->lock);
}

R_API void r_emu_sleeper_free (REmuSleeper *sleeper) {
	if (!sleeper) {
		return;
	}
	r_th_wait (sleeper->me);
	r_th_free (sleeper->me);
	r_th_lock_free (sleeper->lock);
	free (sleeper);
}

#include <r_types.h>
#include <SDL2/SDL.h>

typedef struct interactor_sdl2_user_t {
	SDL_Window *window;
	SDL_Renderer *renderer;
} InteractorSDL2User;

void *sdl2_init () {
	if (SDL_WasInit (SDL_INIT_VIDEO)) {
		return NULL;
	}
	if (SDL_Init (SDL_INIT_VIDEO) < 0) {
		return NULL;
	}
	InteractorSDL2User *user = R_NEW0 (InteractorSDL2User);
	if (!user) {
		SDL_Quit ();
	}
	return user;
}

void sdl2_fini (REmuInteractor *ei) {
	if (!ei || ei->user) {
		return;
	}
	InteractorSDL2User *user = (InteractorSDL2User *)ei->user;
	if (user->renderer) {
		SDL_DestroyRenderer (user->renderer);
	}
	if (user->window) {
		SDL_DestroyWindow (user->window);
	}
	SDL_Quit ();
	free (user);
}

void sdl2_init_screen (REmuInteractor *ei, unsigned short w, unsigned short h) {
	if (!ei || ei->user) {
		return;
	}
	InteractorSDL2User *user = (InteractorSDL2User *)ei->user;
	user->window = SDL_CreateWindow ("SDL2_Interactor", SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, w, h, 0);
	if (user->window) {
		user->renderer = SDL_CreateRenderer (user->window, -1, 0);
	}
	if (!user->renderer) {
		SDL_DestroyWindow (user->window);
		user->window = NULL;
		return;
	}
	SDL_SetRenderDrawColor (user->renderer, 0x30, 0x30, 0x30, 0xff);
	SDL_RenderClear (user->renderer);
	SDL_RenderPresent (user->renderer);
}

REmuInteractorPlugin interactor_sdl2 = {
	.name = "SDL2_Interactor",
	.desc = "Interacts with SDL2",
	.init = sdl2_init,
	.fini = sdl2_fini,
	.init_screen = sdl2_init_screen,
};

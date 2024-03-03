#include <SDL.h>
#include <stdlib.h>
#include <stdbool.h>


#include "debugmalloc.h"
#include "game.h"
#include "menu.h"
#include "score.h"


/* ablak letrehozasa */
void sdl_init(char const *felirat, int szeles, int magas, SDL_Window **pwindow, SDL_Renderer **prenderer) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        SDL_Log("Nem indithato az SDL: %s", SDL_GetError());
        exit(1);
    }
    SDL_Window *window = SDL_CreateWindow(felirat, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, szeles, magas, 0);
    if (window == NULL) {
        SDL_Log("Nem hozhato letre az ablak: %s", SDL_GetError());
        exit(1);
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (renderer == NULL) {
        SDL_Log("Nem hozhato letre a megjelenito: %s", SDL_GetError());
        exit(1);
    }
    SDL_RenderClear(renderer);

    *pwindow = window;
    *prenderer = renderer;
}

/*Ablak bezárása*/
void sdl_close(SDL_Window **pwindow, SDL_Renderer **prenderer) {
    SDL_DestroyRenderer(*prenderer);
    *prenderer = NULL;

    SDL_DestroyWindow(*pwindow);
    *pwindow = NULL;

    SDL_Quit();
}

//Itt kezdődik a kódunk
//A másik modulokból hív meg függvényeket, hogy a játék haladjon körbe
//A menüböl a játékba, a játékból a pont képernyőre a pont képernyőjéből a menübe jusson
int main(int argc, char *argv[]) {
    /* ablak letrehozasa */
    SDL_Window *window;
    SDL_Renderer *renderer;
    sdl_init("Asteroids", 960, 720, &window, &renderer);    //960, 720

    //Kiválasztott nehézség, pontszám, és hogy a játékos kilép e-vagy sem
    int sel;
    unsigned int pts;
    bool exit = false;

    //Amíg nem akarunk kilépni, addig forogjon körbe a játék logikája
    //Alap helyzetben nem kell az "!exit" feltétel, de biztonság kedvéért beírjuk
    while(!exit) {

        sel = -1;
        pts = 0;

        //A menü visszatérési értéke az lesz, amilyen nehézséget választott a játékos, 1-3 között, vagy -1 ha a kilépést válaszotta
        sel = menu(renderer);
        if(sel < 0) break;

        //A fő játék, visszatér azzal, hogy a játékos megnyomta-e az "x" gombot, vagy csak elfogyott az élete
        exit = game(renderer, sel, &pts);
        if(exit) break;

        //A pont képernyő,
        score(pts, renderer);

    }
    /* ablak bezarasa */
    sdl_close(&window, &renderer);
    SDL_Quit();
    return 0;
}

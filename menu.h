#ifndef MENU_H
#define MENU_H

#include <SDL_ttf.h>

//Megrajzol egy új szöveget. Paraméterként veszi a renderert, a kiirandó szöveget, a koordinátáit, a méretét, valamint hogy ez
//egy jelenleg kiválasztott szöveg, vagy sem (rajzoljon-e mellé egy kis hajó szimbólumot)
void Text_Create(SDL_Renderer *renderer, char* text, float x, float y, int size, bool selected);

//A főfüggvény a modulban. A menü elemeinek megjelenítésére szolgál. A main()-által van meghívva
//és visszatér a kiválasztott nehézségi fokozattal (könnyü-nehéz: 1-3), vagy -1-el, ha a játékos a kilépés opciót választotta
int menu(SDL_Renderer *renderer);

#endif

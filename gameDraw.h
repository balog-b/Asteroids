#ifndef GAMEDRAW_H
#define GAMEDRAW_H
#include <SDL2_gfxPrimitives.h>
#include "game.h"

// A deklarált függvények leírása a gameDraw.c-ben megtalálhatók

/** \brief A játékos kirajzolásáért felelős.
 *
 * \param *renderer: SDL renderer
 * \param x: A játékos x koordinátája
 * \param y: A játékos y koordinátája
 * \param r: A hajó orrának iránya
 * \param inv: A halhatatlanságból hátralévő idő
 *
 */
void DrawPlayer(SDL_Renderer *renderer, double x, double y, double r, int inv);

//Végigfut a lövedékek listáján, és kirajzolja azokat. Paraméterként veszi a lövedék lista első elemét
void DrawBullets(SDL_Renderer *renderer, BulletList *eleje);

//Végigfut az aszteroidák listáján, és kirajzolja azokat. Paraméterként veszi az aszteroida lista első elemét
void DrawAsteroids(SDL_Renderer *renderer, AsteroidList *eleje);

//A hajó felrobbanásának effektusa. Paraméterként veszi a játékos utolsó pozícióját, valamint a halál óta eltelt idejét
void DeathEffect(SDL_Renderer *renderer, Vector poi, int frame);

//Kirajzolunk annyi életet a felső sarokba, ahány a játékosnak még visszavan
//Paraméterként kapja a játékos hátralévő életeinek számát
void LifeUpdate(SDL_Renderer *renderer, int life);

#endif

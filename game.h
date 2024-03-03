#ifndef GAME_H
#define GAME_H
#include <SDL.h>

//A következő struktúrákat a gameDraw használja

/** \brief  Egy x megy egy y koordinatat tárol egy 2D vektorhoz.
 */
typedef struct Vector {
    double x, y;
} Vector;

/** \brief  Egy lövedék listaelem adatai.
 *          Tartalmazza a hátralévő idejét, a pozícióját és a sebességvektorát.
 */
typedef struct Bullet {
    int life;
    Vector poi, dir;
} Bullet;

/** \brief  Egy aszteroida listaelem adatai.
 *          Tartalmazza a méretét, az elforgatását, a forgási sebességét, a pozícióját és a sebességvektorát.
 */
typedef struct Asteroid {
    int size;
    double r;
    double r_speed;
    Vector poi, dir;
} Asteroid;

/** \brief  A lövedékek listája.
 *          Lövedék adatokat tartalmaz, valamint a következő listaelemre mutató pointert.
 */
typedef struct BulletList {
    Bullet adat;
    struct BulletList *kov;
} BulletList;

/** \brief  Az aszteroidák listája.
 *          Aszteroida adatokat tartalmaz, valamint a következő listaelemre mutató pointert.
 */
typedef struct AsteroidList {
    Asteroid adat;
    struct AsteroidList *kov;
} AsteroidList;

/** \brief  A modulunk "főfüggvénye". Ezt fogja meghívni a main(), innen fog elkezdődni a játék.
 *
 * \param   *renderer: Az SDL renderer
 * \param   dif: Nehézség (0= Könnyű, 1 = Közepes, 2 = Nehéz)
 * \param   *points: Játékos pontjai
 * \return  True = a játék ki lett "x"-elve, meg lett szakítva. False = a játék végetért, mivel a játékosnak elfogytak az életei.
 */
bool game(SDL_Renderer *renderer, int dif, unsigned int *points);

#endif

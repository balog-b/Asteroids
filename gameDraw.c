#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#include "gameDraw.h"

//Ez a modul tartja karban a kirajzolásokat a játék közben.
//Ez csupán egy segédmodul, az átláthatóság érdekében lettek kigyüjtve ezek a függvények
//Minden függvény ami itt van a game.c-ben van meghívva
//A DrawPlayer() ezen kívül menuben is meg van hívva


void DrawPlayer(SDL_Renderer *renderer, double x, double y, double r, int inv) {

    if(inv % 2 != 0) return;

    float const scale = 20;

    //Kiszámoljuk az abszolut helyeit a pontoknak, a hajó középpontjának abszolut értékének függvényében
    double r1 = M_PI*(r/180.0);
    double x1 = cos(r1)*scale + x;
    double y1 = sin(r1)*scale + y;

    double r2 = M_PI*((r + 140.0)/180.0);
    double x2 = (cos(r2))*scale + x;
    double y2 = (sin(r2))*scale + y;

    double r3 = M_PI*((r + 180.0)/180.0);
    double x3 = (cos(r3))*5 + x;
    double y3 = (sin(r3))*5 + y;

    double r4 = M_PI*((r + 220.0)/180.0);
    double x4 = (cos(r4))*scale + x;
    double y4 = (sin(r4))*scale + y;


    //Minden páratlan számú képkockán a halhatatlanság lejártakor halványabban rajzolunk.
    //Ezzel elérjük, hogy a hajó villogjon, jelezve, hogy halhatatlan
    int color = 255;
    if(inv % 2 != 0) color = 50;

    Sint16 vx[4] = {x1, x2, x3, x4};
    Sint16 vy[4] = {y1, y2, y3, y4};

    polygonRGBA(renderer, vx, vy, 4, 0, color, 0, 255);
}

void DrawBullets(SDL_Renderer *renderer, BulletList *eleje) {
    BulletList *mozgo;
    for (mozgo = eleje; mozgo != NULL; mozgo = mozgo->kov) {
        filledCircleRGBA(renderer, mozgo->adat.poi.x, mozgo->adat.poi.y, 5, 0, 255, 0, 255);
    }
}

void DrawAsteroids(SDL_Renderer *renderer, AsteroidList *eleje) {
    AsteroidList *mozgo;
    for (mozgo = eleje; mozgo != NULL; mozgo = mozgo->kov) {
        //Nagy/kicsi méret szerint adjuk meg a méretét
        int scale;
        if(mozgo->adat.size == 2)
            scale = 35;
        else
            scale = 20;

        Sint16 vx[6];
        Sint16 vy[6];
        double r = 0;
        double rad;
        //Megalkotjuk a
        for(int i = 0; i < 6; i++) {
            rad = M_PI*((r + mozgo->adat.r)/180.0);
            vx[i] = (cos(rad))*scale + mozgo->adat.poi.x;
            vy[i] = (sin(rad))*scale + mozgo->adat.poi.y;

            r += 60.0;
        }
        polygonRGBA(renderer, vx, vy, 6, 0, 255, 0, 255);
    }
}

void DeathEffect(SDL_Renderer *renderer, Vector poi, int frame) {

    int randx;
    int randy;
    double size;
    int alpha;
    //Leteszünk véletlenszerü 5 kört a hajó utolsó pozíciójára koörnyékére. A mérete véletlenszerü de arányosan csökken
    //a lehtséges maximuma és minimuma, ahogy nő a halál ideje óta eltelt képkockák száma. Az átteccősége szintúgy
    for(int i = 0; i <= 5; i++) {
        randx = rand() % 30;
        randy = rand() % 30;
        size = (rand() % 5) + 10 - frame * 0.2;
        alpha = 255 - frame * 5; if(alpha < 0) alpha = 0;
        filledCircleRGBA(renderer, poi.x - 28 + randx, poi.y - 28 + randy, size, 0, 255, 0, alpha);
    }

}


void LifeUpdate(SDL_Renderer *renderer, int life) {

    double offset = 15.0;

    for(int i = 1; i <= life; i++){

        Sint16 vx[4] = {offset, offset + 10, offset, offset - 10};
        Sint16 vy[4] = {5.0, 35.32088, 25.0, 35.32088};

        polygonRGBA(renderer, vx, vy, 4, 0, 255, 0, 255);
    offset += 35.0;
    }
}

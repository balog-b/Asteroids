#ifndef SCORE_H
#define SCORE_H

//A játékos adatait tartalmazó struktúra, amit a fájlba ki és beíráskor használunk
typedef struct Stat {
    char name[4];
    unsigned int score;
} Stat;

/*A modulunk főfüggvénye. Kirajzolja a "Game Over" képet. Meghívja az input_text-et és paraméterként kapja a pontszámot; ebböl a kettőbol
hoz létre egy új játékos adatot, és beleteszi a fájlba, ha benne van az első 11-ben pontszám alapján. */
void score(unsigned int score, SDL_Renderer *renderer);

#endif // SCORE_H

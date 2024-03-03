#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL2_gfxPrimitives.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "menu.h"
#include "score.h"

typedef struct Stat Stat;

//Generál egy input mezőt. Amikor ezt a függvényt hívjuk meg, az megállítja a kód menetét, és nem megy tovább amíg a játékos nem nyom entert (vagy lép ki az "x" gombbal)
//Paraméterként kap, egy karaktertömböt amibe fogja tárolni a szöveget, egy max hosszot, a nagyságát a szöveg területének, a szöveg színét, a betűk méretét és a renderert
//Visszatér egy logikai változóval. True = a játékos helyesen beolvasott, False = a játékos kilépett beolvasás közben az "x"-gombbal
static bool input_text(char *dest, size_t hossz, SDL_Rect teglalap, SDL_Color szoveg, int size, SDL_Renderer *renderer) {

    TTF_Init();
    TTF_Font *font = TTF_OpenFont("LiberationSerif-Regular.ttf", size);

    /* Ez tartalmazza az aktualis szerkesztest */
    char composition[SDL_TEXTEDITINGEVENT_TEXT_SIZE];
    composition[0] = '\0';
    /* Ezt a kirajzolas kozben hasznaljuk */
    char textandcomposition[hossz + SDL_TEXTEDITINGEVENT_TEXT_SIZE + 1];
    /* Max hasznalhato szelesseg */
    int maxw = teglalap.w - 2;
    int maxh = teglalap.h - 2;

    dest[0] = '\0';

    bool enter = false;
    bool kilep = false;

    SDL_StartTextInput();
    while (!kilep && !enter) {
        /* doboz kirajzolasa */
        boxRGBA(renderer, teglalap.x, teglalap.y, teglalap.x + teglalap.w - 1, teglalap.y + teglalap.h - 1, 0, 0, 0, 255);

        /* szoveg kirajzolasa */
        int w;
        strcpy(textandcomposition, dest);
        strcat(textandcomposition, composition);
        if (textandcomposition[0] != '\0') {
            SDL_Surface *felirat = TTF_RenderUTF8_Blended(font, textandcomposition, szoveg);
            SDL_Texture *felirat_t = SDL_CreateTextureFromSurface(renderer, felirat);
            SDL_Rect cel = { teglalap.x, teglalap.y, felirat->w < maxw ? felirat->w : maxw, felirat->h < maxh ? felirat->h : maxh };
            SDL_RenderCopy(renderer, felirat_t, NULL, &cel);
            SDL_FreeSurface(felirat);
            SDL_DestroyTexture(felirat_t);
            w = cel.w;
        } else {
            w = 0;
        }
        /* kurzor kirajzolasa */
        if (w < maxw) {
            vlineRGBA(renderer, teglalap.x + w + 2, teglalap.y + 2, teglalap.y + teglalap.h - 3, szoveg.r, szoveg.g, szoveg.b, 192);
        }
        /* megjeleniti a képernyon az eddig rajzoltakat */
        SDL_RenderPresent(renderer);

        SDL_Event event;
        SDL_WaitEvent(&event);
        switch (event.type) {
            /* Kulonleges karakter */
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_BACKSPACE) {
                    int textlen = strlen(dest);
                    do {
                        if (textlen == 0) {
                            break;
                        }
                        if ((dest[textlen-1] & 0x80) == 0x00)   {
                            /* Egy bajt */
                            dest[textlen-1] = 0x00;
                            break;
                        }
                        if ((dest[textlen-1] & 0xC0) == 0x80) {
                            /* Bajt, egy tobb-bajtos szekvenciabol */
                            dest[textlen-1] = 0x00;
                            textlen--;
                        }
                        if ((dest[textlen-1] & 0xC0) == 0xC0) {
                            /* Egy tobb-bajtos szekvencia elso bajtja */
                            dest[textlen-1] = 0x00;
                            break;
                        }
                    } while(true);
                }
                if (event.key.keysym.sym == SDLK_RETURN) {
                    enter = true;
                }
                break;

            /* A feldolgozott szoveg bemenete */
            case SDL_TEXTINPUT:
                if (strlen(dest) + strlen(event.text.text) < hossz) {
                    strcat(dest, event.text.text);
                }

                /* Az eddigi szerkesztes torolheto */
                composition[0] = '\0';
                break;

            /* Szoveg szerkesztese */
            case SDL_TEXTEDITING:
                strcpy(composition, event.edit.text);
                break;

            case SDL_QUIT:
                /* visszatesszuk a sorba ezt az eventet, mert
                 * sok mindent nem tudunk vele kezdeni */
                SDL_PushEvent(&event);
                kilep = true;
                return -1;
        }
    }

    /* igaz jelzi a helyes beolvasast; = ha enter miatt allt meg a ciklus */
    TTF_CloseFont(font);
    SDL_StopTextInput();
    return enter;
}

//Ez a függvény rendezi a játékosok listáját. 1-11 elemet kaphat összesen (tud többet is, de az előző logikák szerint csak annyit fog).
//A rendezés típusa "selection sort"
void Rendezes(Stat *list, int n) {
    for (int i = 0; i < n-1; ++i) {
        int most = i;
        for (int j = i+1; j < n; ++j)
            if (list[j].score > list[most].score)
                most = j;

        if (most != i) {
            Stat temp = list[most];
            list[most] = list[i];
            list[i] = temp;
        }
    }
}


void score(unsigned int score, SDL_Renderer *renderer) {
    /*Deklaralasok*/
    char szoveg[4] = "   ";
    char tscore[10];
    //Text_Create fuggveny definialasa a "menu.c"-ben talalhato
    Text_Create(renderer, "Game Over!", 480, 200, 64, false);

    Text_Create(renderer, "Your score:", 400, 400, 32, false);
    /*Atalakitjuk a pontunkant int-bol string-be, hogy kiirhassuk a kepernyore*/
    sprintf(tscore, "%d", score);
    Text_Create(renderer, tscore, 560, 400, 32, false);

    Text_Create(renderer, "Enter Name:", 400, 480, 32, false);

    SDL_Color zold = {0, 255, 0};
    SDL_Rect r = { 540, 460, 180, 48 };
    //Futtatjuk a szöveg bekérését. Visszatér azzal, hogy a játékos entert nyomott-e(, vagy kilépett)
    //Csak 3 betűs neveket engedünk meg
    input_text(szoveg, 4, r, zold, 48, renderer);

    /*Megnyittjuk a ranglistánkat tartalmazó fájlt*/
    FILE *fp;
    fp = fopen("leaderboard.txt", "r");
    if (fp == NULL) {
        perror("Fájl megnyitása sikertelen"); return; }

    /*Először kiolvassuk az összes elemet, majd az új elemmel együtt rendezzük és visszaírjuk*/

    char str_score[10];
    Stat list[11];
    int n = 0;

    /*Feltöltjük a játékos nevét space-el ha nem írt ki 3 betűs nevet*/
    for(int i = 0; i < 3; i++)
        if(szoveg[i] == '\0') szoveg[i] = ' ';

    //Kiolvassuk az első 10, vagy ha 10-nél kevesebb akkor az összes nevet
    //Berakjuk a listába mind a nevét és a pontszámát, egy struktúrába
    while(fgets(list[n].name, 4, fp) != NULL && n < 10) {

        fgets(str_score, 10, fp);
        list[n].score = atoi(str_score);
        n++;
    }

    fclose(fp);

    //A lista végére tesszük az mostani új pontszámot
    strcpy(list[n].name, szoveg);
    list[n].score = score;

    Rendezes(list, n+1);

    //Töröljük a fájlt és visszaírjuk az új adatokat
    fp = fopen("leaderboard.txt", "w");
    for(int i = 0; i < n + 1; i++ ) {
        fprintf(fp, "%s %d\n", list[i].name, list[i].score);
    }

    fclose(fp);
}


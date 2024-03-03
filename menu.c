
#include <SDL_ttf.h>
#include <SDL2_gfxPrimitives.h>
#include <stdbool.h>
#include <stdio.h>

#include "menu.h"
#include "gameDraw.h"


void Text_Create(SDL_Renderer *renderer, char* text, float x, float y, int size, bool selected) {

    TTF_Init();
    TTF_Font *font = TTF_OpenFont("LiberationSerif-Regular.ttf", size);
    if (!font) {
        SDL_Log("Nem sikerult megnyitni a fontot! %s\n", TTF_GetError());
        exit(1);
    }

    SDL_Surface *felirat;
    SDL_Texture *felirat_t;
    SDL_Rect hova = { 0, 0, 0, 0};
    SDL_Color c = {0, 255, 0};
    felirat = TTF_RenderUTF8_Solid(font, text, c);
    felirat_t = SDL_CreateTextureFromSurface(renderer, felirat);

    //Mivel a megadott koordináta alaphejzetben a szöveg bal felső sarka, ezért nehéz a szöveget jól elhelyezni
    //Ezért pl "x koordináta - szélesség / 2;"
    //Ezzel a kiirandó szöveg közepe lesz a megadott koordinátán
    hova.x = x - (felirat->w / 2);
    hova.y = y - (felirat->h / 2);
    hova.w = felirat->w;
    hova.h = felirat->h;

    //Ha ez a kiválasztott menüpont. Rajzolunk egy hajót
    if(selected) DrawPlayer(renderer, x - (felirat->w / 2 + 30), y, 0, 0);

    SDL_RenderCopy(renderer, felirat_t, NULL, &hova);
    SDL_FreeSurface(felirat);
    SDL_DestroyTexture(felirat_t);
    TTF_CloseFont(font);
}

//A menü kinézetét alkotja meg a text alkotó függvény segítségével
static void MenuSelect(SDL_Renderer *renderer, int x) {

    //Letöröljük ami eddig a képernyőn volt
    boxRGBA(renderer, 0, 0, 960, 720, 0, 0, 0, 255);

    bool select_def[4] = {false, false, false, false};
    //Megnézzük melyik menüpont volt kiválasztva, és a szerint küldjük tovább a függvény 6. argumentumát
    select_def[x] = true;

    Text_Create(renderer, "Asteroids", 480, 200, 128, false);

    Text_Create(renderer, "Play", 480, 370, 32, select_def[0]);
    Text_Create(renderer, "Controls", 480, 410, 32, select_def[1]);
    Text_Create(renderer, "Leaderboard", 480, 450, 32, select_def[2]);
    Text_Create(renderer, "Exit", 480, 490, 32, select_def[3]);
}

//A 3 nehézségi fokozatot jeleníti meg a text alkotó függvény segítségével
static void DifSelect(SDL_Renderer *renderer, int x) {

    //Letöröljük ami eddig a képernyőn volt
    boxRGBA(renderer, 0, 0, 960, 720, 0, 0, 0, 255);

    bool select_def[3] = {false, false, false};
    //Megnézzük melyik menüpont volt kiválasztva, és a szerint küldjük tovább a függvény 6. argumentumát
    select_def[x] = true;

    Text_Create(renderer, "Easy", 480, 300, 32, select_def[0]);
    Text_Create(renderer, "Medium", 480, 360, 32, select_def[1]);
    Text_Create(renderer, "Hard", 480, 420, 32, select_def[2]);
}

//A "Controls" oldalt jeleníti meg a text alkotó függvény segítségével
static void Tutorial(SDL_Renderer *renderer) {

    //Letöröljük ami eddig a képernyőn volt
    boxRGBA(renderer, 0, 0, 960, 720, 0, 0, 0, 255);

    Text_Create(renderer, "Controls", 480, 120, 64, false);
    Text_Create(renderer, "W: Accelerate", 480, 260, 32, false);
    Text_Create(renderer, "A, D: Steer", 480, 300, 32, false);
    Text_Create(renderer, "Spacebar: Shoot", 480, 340, 32, false);

    Text_Create(renderer, "Avoid the asteroids and the edge of the area", 480, 400, 32, false);
    Text_Create(renderer, "Destroy asteroids and survive to earn points", 480, 440, 32, false);

    Text_Create(renderer, "Back", 480, 600, 32, true);

}

//A ranglistát jeleníti meg a text alkotó függvény segítségével
static void Leaderboard(SDL_Renderer *renderer) {

    //Letöröljük ami eddig a képernyőn volt
    boxRGBA(renderer, 0, 0, 960, 720, 0, 0, 0, 255);

    FILE *fp;
    fp = fopen("leaderboard.txt", "r");
    if (fp == NULL) {
        perror("Fájl megnyitása sikertelen");
        return;
    }

    char player[4];
    char score[10];

    int counter = 1;
    char pos[3];
    int offset = 0;

    Text_Create(renderer, "Leaderboard", 480, 60, 64, false);
    //Kiirjuk az elemeket sorban. Mivel beíráskor már rendeztük, ezért itt elég csak kiírjuk őket
    while( fgets(player, 4, fp) != NULL && counter <= 10 ) {

        fgets(score, 10, fp);

        sprintf(pos, "%d", counter);
        score[strlen(score) - 1] = '\0';


        Text_Create(renderer, pos, 120, 120 + offset, 32, false);
        Text_Create(renderer, player, 200, 120 + offset, 32, false);

        Text_Create(renderer, score, 740, 120 + offset, 32, false);

        offset += 45;
        counter++;
    }
    fclose(fp);

    //A maradék helyet feltöltjük helytartó szimbólumokkal
    while(counter <= 10) {

        Text_Create(renderer, "--", 120, 120 + offset, 32, false);
        Text_Create(renderer, "---", 200, 120 + offset, 32, false);
        Text_Create(renderer, "0", 740, 120 + offset, 32, false);

        offset += 45;
        counter++;
    }

    //Innen csak ezen a gombon tud lenni a játékos. Odarajzoljuk a hajót
    Text_Create(renderer, "Back", 480, 600, 32, true);
}


int menu(SDL_Renderer *renderer) {

    //Kiválasztott menüpont. Helyenként több vagy kevesebbet léphetünk
    int selector = 0;
    //Jelenleg megjelenített menürész. Ezt veszi figyelembe a ciklusunk, hogy eldöntse hogyan reagáljon a gombok lenyomására
    //a menü
    int menu = 0;

    MenuSelect(renderer, selector);

    SDL_RenderPresent(renderer);

    SDL_Event ev;
    /*Gomb lenyomasakor frissul a kep; */
    while (SDL_WaitEvent(&ev) && ev.type != SDL_QUIT) {
        switch(ev.type) {

            case SDL_KEYDOWN:
                /*Attol fuggoen hogy melyik menupontban vagyunk, a gomb lenyomas mas hatassal van a menure, ezert eloszor megnezzuk melyik menupontban vagyunk,
                es azutan nezzuk meg melyik gomb volt lenyomva. Fontos, mert bizonyos menupontokban pl nem lehet fel le lepni, vagy kevesebb a menupont*/
                switch(menu) {
                    /*Main*/
                    case 0:
                        MenuSelect(renderer, selector);
                        switch(ev.key.keysym.sym) {
                            /*Select*/
                            case SDLK_RETURN:
                                switch(selector) {
                                    /*Play*/
                                    case 0:
                                        menu = 1;
                                        selector = 0;
                                        DifSelect(renderer, selector);
                                        break;
                                    /*Controls*/
                                    case 1:
                                        menu = 2;
                                        Tutorial(renderer);
                                        break;
                                    /*Leaderboard*/
                                    case 2:
                                        menu = 2;
                                        Leaderboard(renderer);
                                        break;
                                    /*Exit*/
                                    case 3:
                                        return -1;
                                        break;
                                }
                                break;
                            /*Up*/
                            case SDLK_w: if(selector > 0) { selector -= 1; MenuSelect(renderer, selector); } break;
                            /*Down*/
                            case SDLK_s: if(selector < 3) { selector += 1; MenuSelect(renderer, selector); } break;
                            }

                        break;
                    /*Difficulty select*/
                    case 1:
                        DifSelect(renderer, selector);
                        switch(ev.key.keysym.sym) {
                            /*Select*/
                            case SDLK_RETURN:
                                /*Kissebb rovidites, mivel ha entert nyom csakis a jatekot kezdheti el, a selector most a kivalasztott nehesseget tukrozi,
                                eleg ha visszaadjuk, amikor ezen a kepernyon entert nyom a jatekos*/
                                return selector;
                                break;
                            /*Up*/
                            case SDLK_w: if(selector > 0) { selector -= 1; DifSelect(renderer, selector); } break;
                            /*Down*/
                            case SDLK_s: if(selector < 2) { selector += 1; DifSelect(renderer, selector); } break;
                            case SDLK_ESCAPE:
                                menu = 0;
                                selector = 0;
                                MenuSelect(renderer, selector);
                                break;
                            }

                        break;
                    /*Controls & Leaderboard*/
                    case 2:
                        //Mivel ezen a két helyen lényegében ugyanazokatt tudjuk csinálni (nem tudunk fel le lépni és vissza lehet menni)
                        //Ezert a kettő menüpont egy helyre lett gyűjtve
                        if(ev.key.keysym.sym == SDLK_RETURN || ev.key.keysym.sym == SDLK_ESCAPE) {
                            menu = 0;
                            MenuSelect(renderer, selector);
                        }
                        break;
                }
                break;
            }
        SDL_RenderPresent(renderer);
    }
    return -1;
}


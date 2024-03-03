#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "debugmalloc.h"
#include "game.h"
#include "gameDraw.h"
#include "menu.h"


/** \brief  Időzítőt állít, ami meghív egy event-et a parameterkent kapott idokben
 *          Ezt a függvényt az SDL_AddTimer függvény használja
 *
 * \param Az idő a következő meghívásig
 * \return Az idő a következő meghívásig
 *
 */
Uint32 idozit(Uint32 ms, void *param) {
    SDL_Event ev;
    ev.type = SDL_USEREVENT;
    SDL_PushEvent(&ev);
    return ms;   /* ujabb varakozas */
}

/*A struktúrák definíciói a game.h-ban megtalálhatók, a magyarázatukkal együtt*/

/** \brief  A robbanások adatai
 *          Tartalmazza a hátralévő idejét, és a pozícióját
 */
typedef struct Explosions {
    int life;
    Vector poi;
} Explosions;

/** \brief  A robbanások listája.
 *          Robbanás adatokat tartalmaz, valamint a következő listaelemre mutató pointert
 */
typedef struct ExplosionList {
    Explosions adat;
    struct ExplosionList *kov;
} ExplosionList;

/** \brief  A játékos adatai.
 *          Tartalmazza a elenlegi pozícióját, a sebességvektorát, a hajó orrának az irányát, valamint, hogy a halhatatlanságból hátralévő idejét
 */
typedef struct Player {
    /*poi: jelenlegi pozíciója, dir: sebesség vektora*/
    Vector poi, dir;
    // A hajó orrának iránya. A 270° a felfele
    double rotation;
    // Hány képkockányi halhatatlansága van a játékosnak
    int inv;
} Player;

/** \brief  A lenyomott billentyűk.
 *          Tartalmazza az előre, jobbra, balra, lövés és szünet gombok állapotát egy logikai változóban
 */
typedef struct Keys {
    bool elore, jobbra, balra, loves, pause;
} Keys;

/**<  Amely függvények minden képkockán meg vannak hívva, azoknak annyi feltétele van, hogy a játék ne legyen szüneteltetve. Egyébként mindig meg vanna hívva*/

/** \brief A játékos megidézése (mikor elkezdődik a játék, vagy mikor meghalt)
 *
 * \param ply: A játékos adatainak struktúrája
 * \return A játékos adatainak struktúrájával
 *
 */
static Player Spawn(Player ply) {

    //A képernyő közepére helyezés
    ply.poi.x = 480.0;
    ply.poi.y = 360.0;

    //Be-, vagy újraállítjuk a sebességét 0-ra
    ply.dir.x = 0.0;
    ply.dir.y = 0.0;

    /*Alapértelmezett "felfelé": 270 fok (mivel most -y van felfelé és +y lefelé)*/
    ply.rotation = 270.0;

    //100 képkockáig halhatatlan
    ply.inv = 100;

    return ply;
}


 /** \brief A játékos mozgásának kiszámolása
  *         Minden képkockán meg van hívva
  *
  * \param old: A játékos mostani sebességét
  * \param mozog: Jelenleg gyorsul e vagy sem
  * \param r: A hajónak az orrának az iránya
  * \return A játékos új sebességével
  *
  */
static Vector Move(Vector old, bool mozog, double r) {
    //Alapértelmezett sebesség
    Vector current;
    current.x = 0;
    current.y = 0;

    //A jelenlegi gyorsulás. Ha a játékos nem mozog, akkor alapértelmezetten 0.
    double a = 0.0;
    double b = 0.0;

    const float speed = 0.5;

    //Ha a hajó épp mozog, ki kell számoljuk, hogy melyik irányba mozog, majd hozzáadni a gyorsulás vektorjahoz
    if(mozog) {
    double rad = M_PI*(r/180.0);
    a = cos(rad);
    b = sin(rad);
    }

    //Az új sebesség kiszámolása. A gyorsulás folyamatosan csökken képkockánként, így a hajó "csúszik" tovább miután elengedjük a gázt
    current.x = (old.x + a*speed) * 0.935;
    current.y = (old.y + b*speed) * 0.935;

    return current;
}

 /** \brief Elpusztítja a paraméterként kapott címen lévő lövedéket.
  *
  * \param **element: A törlendő lövedék címe
  * \param **eleje: A lista elejének a címe
  * \param **elozo: Törlendő lövedék elötti elem címét
  *
  */
static void DestroyBullet(BulletList **element, BulletList **eleje, BulletList **elozo) {

    //Ha nem létezik, akkor valami hiba történt, ne futtasson semmi változtatást
    if(*element == NULL);

    //Ha nem létezik az előző elem, akkor az első elemet töröljük jelenleg
    else if(*elozo == NULL) {

        //Lementjük a jelenlegi elem következő tagját, és felszabadítjuk az elemet
        BulletList *uj_eleje = (*element)->kov;
        free(*element);

        //Az új eleje mostmár az eleje a listának, és amire ő mutat lesz a következő elem
        //(emiatt a függvény után a lista körbejárásakor nem kell a mozgatót léptetni)
        *eleje = uj_eleje;
        *element = *eleje;
    }
    //Ha van előző, akkor a függvény közepéről törlünk jelenleg
    else if(*elozo != NULL) {

        //Elmentjük a mozgo következő tagját, hogy késöbb visszaadjuk neki
        BulletList *kov = (*element)->kov;

        //Az előző elemet átkötjük a mozgóba, felülhidalva a free()-elt lista elemet
        (*elozo)->kov = kov;
        free(*element);
        *element = kov;
    }
}


 /** \brief Elpusztítja a paraméterként kapott címen lévő aszteroidát.
  *
  * \param **element: A törlendő aszteroida címe
  * \param **eleje: A lista elejének a címe
  * \param **elozo: Törlendő aszteroida elötti elem címét
  *
  */
static void DestroyAsteroid(AsteroidList **element, AsteroidList **eleje, AsteroidList **elozo) {

    //Ha nem létezik, akkor valami hiba történt, ne futtasson semmi változtatást
    if(*element == NULL);

    //Ha nem létezik az előző elem, akkor az első elemet töröljük jelenleg
    else if(*elozo == NULL) {

        //Lementjük a jelenlegi elem következő tagját, és felszabadítjuk az elemet
        AsteroidList *uj_eleje = (*element)->kov;
        free(*element);

        //Az új eleje mostmár az eleje a listának, és amire ő mutat lesz a következő elem
        //(emiatt a függvény után a lista körbejárásakor nem kell a mozgatót léptetni)
        *eleje = uj_eleje;
        *element = *eleje;
    }

    //Ha van előző, akkor a függvény közepéről törlünk jelenleg
    else if(*elozo != NULL) {

        //Elmentjük a mozgo következő tagját, hogy késöbb visszaadjuk neki
        AsteroidList *kov = (*element)->kov;

        //Az előző elemet átkötjük a mozgóba, felülhidalva a free()-elt lista elemet
        (*elozo)->kov = kov;
        free(*element);
        *element = kov;
    }
}

 /** \brief Új robbanás effektust hoz létre.
  *
  * \param element: jelenleg felrobbanó aszteroidának adatait
  * \param **eleje: A lista elejének a címe
  *
  */
static void Explode(Asteroid element, ExplosionList **eleje) {

    ExplosionList *new_e;
    new_e = (ExplosionList*) malloc(sizeof(ExplosionList));
    new_e->kov = *eleje;

    //A robbanás helye az aszteroida helye. Az élete szabja meg, hogy hány képkockáig lesz a képernyőn
    new_e->adat.poi.x = element.poi.x;
    new_e->adat.poi.y = element.poi.y;
    new_e->adat.life = 100;

    //A láncolt lista elejére beszúrjuk
    *eleje = new_e;
}


 /** \brief A lövedékek listájának menedzselése és a lövedékek mozgásának kiszámolásáért felelős. (Új lövedék alkotása, mozgatása, pusztítása)
  *         Minden képkockán meg van hívva
  *
  * \param  *eleje: A lista elejének a címe
  * \param  shoot: A játékos lő-e éppen
  * \param  r: A hajónak az orrának az iránya
  * \param  poi: A hajó pozíciója
  * \param  dir: A hajó jelenlegi sebessége
  * \return A lövedék lista új eleje
  */
static BulletList *Bullet_m(BulletList *eleje, bool shoot, double r, Vector poi, Vector dir) {

    //Lövés esetén új lövedéket alkotunk
    if(shoot) {
        BulletList *new_b;
        new_b = (BulletList*) malloc(sizeof(BulletList));
        //Előre beszúrós lista elem
        new_b->kov = eleje;
        //Ennyi képkockáig fog létezni a lövedék, mielött eltűnik
        new_b->adat.life = 30;

        //Az új lövedék irányvektorát kiszámítjuk, a hajó orrának függvényében
        double r1 = M_PI*(r/180.0);
        double x1 = cos(r1);
        double y1 = sin(r1);

        //Feltöltjük az adatokkal a lista elemet.
        //A lövés sebessége valamilyen szinten fel van gyorsítva a hajó sebessége által. Ez fontos, hogy a hajó ne hagyja le a saját lövedékét,
        //vagy hogy csak simán ne érződjőn túl lassúnak a lövedék mozgás közben.
        new_b->adat.dir.x = x1 + dir.x * 0.1;
        new_b->adat.dir.y = y1 + dir.y * 0.1;
        //Lövedék kezdő pozíciója -> hajó pozíciója + egyszer a sebessege, hogy eegy kicsit előrréb jelenjen meg mint a hajó
        new_b->adat.poi.x = poi.x + new_b->adat.dir.x;
        new_b->adat.poi.y = poi.y + new_b->adat.dir.y;

        eleje = new_b;
    }

    //A lista elemeken végigfutunk
    BulletList *mozgo;
    BulletList *elozo;
    mozgo = eleje;
    elozo = NULL;
    while (mozgo != NULL) {
            //Minden elemböl levonunk 1 életet
            mozgo->adat.life -= 1;
        //Ha elfogyott az élete a lövedéknek, szedjük ki a listábol
        if (mozgo->adat.life <= 0) {
            DestroyBullet(&mozgo, &eleje, &elozo);
            //Ha a lövedék elpusztult, már nincs értelme a lövedék mozgatásának, ezért a ciklus elejére lépünk
            //A DestroyBullet()-ben már meg volt a mozgó pointer léptetése, (az előző meg nem fog változni), szóval amiatt nem kell aggódni
            continue;
        }

        //Pozició + sebesség * egy konstans, hogy jó legyen a sebessége a lövedéknek
        mozgo->adat.poi.x += mozgo->adat.dir.x * 12;
        mozgo->adat.poi.y += mozgo->adat.dir.y * 12;

        //A lemaradó mozgó figyeli az előző elemet, abban az esetben ha a lista közepéről kéne törölni egy elemet
        elozo = mozgo;
        mozgo = mozgo->kov;
    }
    //Visszatérünk az új elejével
    return eleje;
}


 /** \brief Az aszteroidák listájának menedzselése és az aszteroidák mozgásának kiszámolásáért felelős. (Új aszteroida alkotása, mozgatása, pusztítása)
  *         Minden képkockán meg van hívva
  *
  * \param  *eleje: A aszteroid lista eleje
  * \param  *bullets: A lövedék lista elej
  * \param  *chance: Esély arra, hogy új aszteroida jelenjen meg
  * \param  chance_modifier: Esély változó, nehézségtől függő
  * \param  speed_mod: Sebesség változó, nehézségtől függő
  * \return Az aszteroida lista új eleje
  */
static AsteroidList *Asteroid_m(AsteroidList *eleje, BulletList *bullets, int *chance, int chance_modifier, float speed_mod) {

    /*Minden képkockán van esély arra, hogy megjelenjen egy aszteroida. Ha az 1 és 1000 közötti véletlenszerű szám kisebb, mint az esély változó,
    akkor alkosson egy új aszteroidát. Az esély minden képkockán nő, ezért idővel csak-csak meg fog jelenni egy aszteroida, de ez eléggé
    véletlenszerű. Röviden; minél több ideje volt, hogy megjelent egy aszteroida, annál több esély van arra, hogy megjelenjen egy*/
    if(rand() % 1000 + 1 < *chance) {
        //Új lista elemet adunk az aszteroidákhoz
        AsteroidList *new_a;
        new_a = (AsteroidList*) malloc(sizeof(AsteroidList));

        //1/3 esély van arra, hogy az aszteroida egy nagy aszteroida legyen. 2/3 arra, hogy egy kicsi
        int sizerand = rand() % 3 + 1;
        if(sizerand <= 2)
            new_a->adat.size = 1;
        if(sizerand >= 3)
            new_a->adat.size = 2;

        double px;
        double py;
        double dx;
        double dy;

        //Véletlenszerűen veszünk egy oldalt. Ezzel mondjuk meg, hogy melyik oldalán jelenjen meg a képernyőn
        int siderand = rand() % 4 + 1;
        //Véletlenszerűen veszünk egy merőleges irányt. Ezt később használjuk
        float dirrand = ((rand() % 21) * 0.1) - 1;
        switch(siderand) {
            //Az aszteroida balrol jelenjen meg
            case 1:
                //px: Az aszteroida 30-al a képen kívül jelenjen meg
                //py: Az aszteroida y koordinátája véletlenszerű, 10 és 710 között van
                //dx: Az x sebessége. Hozzáadjuk a véletlenszerűen vett sebesség abszolútértékét, hogy ne legyen az összes aszteroida x sebessége ugyanaz
                //dy: Az y sebessége, ez a véletlenszerű -1.0 és 1.0 közötti számunk
                px = -30.0;
                py = 10.0 + rand() % 700;
                dx = 1.0 + abs(dirrand);
                dy = 0.0 + dirrand;
                break;
            //Az aszteroida felül jelenjen meg
            case 2:
                px = 10 + rand() % 940;
                py = -30.0;
                dx = 0.0 + dirrand;
                dy = 1.0 + abs(dirrand);
                break;
            //Az aszteroida jobbrol jelenjen meg
            case 3:
                px = 990.0;
                py = 10.0 + rand() % 700;
                dx = -1.0 + abs(dirrand);
                dy = 0.0 + dirrand;
                break;
            //Az aszteroida alulról jelenjen meg
            case 4:
                px = 10.0 + rand() % 940;
                py = 750.0;
                dx = 0.0 + dirrand;
                dy = -1.0 + abs(dirrand);
                break;
            }

        /*Lecsekkoljuk, hogy a véletlenszerűleg létrehozott aszteroidánk helyén van-e lövedék jelenleg.
        Ha igen, akkor a lefoglalt területet felszabadítjuk és nem alkotunk új aszteroidát
        Ez a rész egy hiba elháritására van, ahol a kód néha hibázik ha az épp megalakuló aszteroidát felrobbantjuk*/
        bool obstruct = false;
        BulletList *b_mozgo;
        b_mozgo = bullets;

            while(b_mozgo != NULL) {
                float d = sqrt(pow(b_mozgo->adat.poi.x - px ,2) + pow(b_mozgo->adat.poi.y - py, 2));
                if(d < 40)  {
                    obstruct = true;
                    break;
                }
                b_mozgo = b_mozgo->kov;
            }
            if(obstruct) free(new_a);
            else {
            //Ha nincs lövedék a helyén, beírjuk az új adatokat
            new_a->kov = eleje;
            new_a->adat.dir.x = dx * speed_mod;
            new_a->adat.dir.y = dy * speed_mod;
            new_a->adat.poi.x = px;
            new_a->adat.poi.y = py;
            //Véletlenszerű forgási sebesség
            new_a->adat.r = 0;
            new_a->adat.r_speed = (((rand() % 21) * 0.1) - 1.0) * 5;

            eleje = new_a;

            //Újraindítjuk az esély változót, hogy ne jelenjen meg aszteroida túl hamar
            *chance = 0;

         }
    }
    //Az esély gyorsabban nő nagyobb nehézségi fokozatokon. Ezt az esély módosító szabja meg
    *chance += 2  + chance_modifier;

    //A lista elemeken végigfutunk
    AsteroidList *mozgo;
    AsteroidList *elozo;
    mozgo = eleje;
    elozo = NULL;

    while (mozgo != NULL) {
        //Megnézzük kiment-e a képernyőböl az aszteroida
        if(mozgo->adat.poi.x <= -35 || mozgo->adat.poi.x >= 995 || mozgo->adat.poi.y <= -35 || mozgo->adat.poi.y >= 755) {
            //Ha igen, elpusztítjuk
            DestroyAsteroid(&mozgo, &eleje, &elozo);
            continue;
        }

        //Frissítjük az aszteroida koordinátáját a sebessége függvényében, a forgását szintúgy
        mozgo->adat.poi.x += mozgo->adat.dir.x * 2;
        mozgo->adat.poi.y += mozgo->adat.dir.y * 2;
        mozgo->adat.r += mozgo->adat.r_speed;

        //Léptetéskor megtartjuk az elötte lévő elemet, hogy elem törlése esetén fel tudjuk használni
        elozo = mozgo;
        mozgo = mozgo->kov;
    }
    return eleje;
}


 /** \brief A robbanások robbanás listájának menedzseléséért, valamint a robbanások megjelenítéséért felelős.
  *         Minden képkockán meg van hívva
  *
  * \param  *renderer: Az SDL renderer
  * \param  *eleje: A robbanás lista első eleme
  * \return A robbanás lista új eleje
  */
static ExplosionList *Explosion_m(SDL_Renderer *renderer, ExplosionList *eleje) {

    //Végigfutunk a láncolt listán
    ExplosionList *mozgo;
    ExplosionList *elozo;
    mozgo = eleje;
    elozo = NULL;

    while (mozgo != NULL) {

        //Redukáljuk a maradék idejét a robbanáseffektusnak
        mozgo->adat.life -= 5;

        //Ha elfogyott az élete, töröljük (csak ebben a függvényben törlünk robbanás lista elemet, ezért nem szükséges új függvénybe tenni)
        if (mozgo->adat.life <= 0) {

            //Ha nem létezik, akkor valami hiba történt, ne futtasson semmi változtatást
            if(mozgo == NULL);

            //Ha nem létezik az előző elem, akkor az első elemet töröljük jelenleg
            else if(elozo == NULL) {
                //Lementjük a jelenlegi elem következő tagját, és felszabadítjuk az elemet
                ExplosionList *uj_eleje = mozgo->kov;
                free(mozgo);

                //Az új eleje mostmár az eleje a listának, és amire ő mutat lesz a következő elem
                //(emiatt a függvény után a lista körbejárásakor nem kell a mozgatót léptetni)
                eleje = uj_eleje;
                mozgo = eleje;
            }

            //Ha van előző, akkor a függvény közepéről törlünk jelenleg
            else if(elozo != NULL) {

                //Elmentjük a mozgo következő tagját, hogy késöbb visszaadjuk neki
                ExplosionList *kov = mozgo->kov;

                //Az előző elemet átkötjük a mozgóba, felülhidalva a free()-elt lista elemet
                elozo->kov = kov;
                free(mozgo);
                mozgo = kov;
            }
            continue;
        }

        //A robbanás jelenlegi mérete és áttetszősége a maradék idejével arányos. Kiszámoljuk mik lesznek ezek
        //Ezzel azt érjük el, hogy az effektusunk egyre nagyobb és halványabb lesz minden képkockán
        float size = 10 + (100 - mozgo->adat.life);
        int alpha = mozgo->adat.life*2;
        //Megrajzoljuk a robbanást
        filledCircleRGBA(renderer, mozgo->adat.poi.x, mozgo->adat.poi.y, size, 0, 255, 0, alpha);

        //Léptetjük a mozgó pointert. A lemaradó pointer figyeli az elötte lévő tagot, hogy törlés esetén megtaláljuk azt
        elozo = mozgo;
        mozgo = mozgo->kov;
    }

    //Visszatérünk az új elejével
    return eleje;
}


 /** \brief Két kicsi aszteroidát idéz a paraméterként kapott aszteroida helyére.
  *         Ezt használjuk mindig, mikor egy nagy aszteroida felrobban és két kicsire bomlik
  *
  * \param  element: Az aszteroida adatai, ahova akarjuk a két új aszteroidát
  * \param  **eleje: Az aszteroida lista első eleme
  */
static void Split(Asteroid element, AsteroidList **eleje) {
    //2x idézünk egy aszteroidát
    for(int i = 0; i < 2; i++) {
        AsteroidList *new_a;
        new_a = (AsteroidList*) malloc(sizeof(AsteroidList));

        //Csak kicsi méretű lehet
        new_a->adat.size = 1;

        //Alkotunk 2 véletlenszerű számot -1.0 és 1.0 között
        float dirrandx = ((rand() % 21) * 0.1) - 1;
        float dirrandy = ((rand() % 21) * 0.1) - 1;

        //A pozíciójuk a paraméterként kapott aszteroida pozíciója, az irányuk a véletlenszerű változónk
        double px = element.poi.x;
        double py = element.poi.y;
        double dx = dirrandx;
        double dy = dirrandy;

        new_a->adat.dir.x = dx;
        new_a->adat.dir.y = dy;
        new_a->adat.poi.x = px;
        new_a->adat.poi.y = py;

        //A forgási sebességük, szintén véletlenszerű
        new_a->adat.r = 0;
        new_a->adat.r_speed = (((rand() % 21) * 0.1) - 1.0) * 7.5;

        //A lista elejébe beszúrjuk
        new_a->kov = *eleje;
        *eleje = new_a;
    }
}


 /** \brief A Collision függvénynek 2 funkciója van. Teszteli, hogy a hajó nekiment-e bármelyik aszteroidának, vagy bármely aszteroida érintkezik-e egy lövedékkel.
  *         Minden képkockán meg van hívva
  *
  * \param  **asteroids: Az aszteroida lista első elemének a címe
  * \param  **bullets: A lövedékek lista első elemének a címe
  * \param  **explosions: A robbanások lista első elemének a címe
  * \param  inv: A játékos halhatatlanságából hátralévő idő
  * \param  *points: A pontszáma a játékosnak
  * \return A játékos nekiment-e egy aszteroidának, vagy a képernyő széléhez
  */
static bool Collision(Vector poi, AsteroidList **asteroids, BulletList **bullets, ExplosionList **explosions,  int inv, unsigned int *points) {

    //A hajónk mérete, "hitbox"-a
    int const shipsize = 20;
    /*Megnezzuk, hogy a jatekos hozzáér-e a képernyő széléhez*/
    if(poi.x + shipsize > 960 || poi.x - shipsize < 0 || poi.y + shipsize > 720 || poi.y - shipsize < 0)
        /*return true -> a hajó fel kell robbanjon*/
        return true;


    //Aszteroida érintkezés érzékelőjének nagysága
    double r;
    //Táv, az aszteroida és a hajó vagy a lövedék között
    double d;
    AsteroidList *a_mozgo;
    AsteroidList *a_elozo;
    a_elozo = NULL;
    a_mozgo = *asteroids;
    //Elpusztzult-e a ciklusban lévő aszteroida
    bool destroy = false;

    //Végigfutunk az aszteroidák láncolt listáján
    while (a_mozgo != NULL) {
        /*Aszteroida érintkezés érzékelőjének nagysága különbözö, ha nagy vagy kicsi az aszteroida, ezért úgy adjuk meg*/
        if(a_mozgo->adat.size == 2) r = 35;
        else r = 18;
        //Megnézzuk van e olyan aszteroida amelyik hozzaer a jatekoshoz, ha a játékos nem halhatatlan
        if(inv <= 0) {

            //Távolság a hajó és az aszteroida között
            d = sqrt(pow(poi.x - a_mozgo->adat.poi.x ,2) + pow(poi.y - a_mozgo->adat.poi.y, 2));
            if(d < r + shipsize) return true;
        }
        //Megnézzük, hogy van-e olyan lövedék ami érintkezik a jelenleg vizsgált aszteroidával
        BulletList *b_mozgo;
        BulletList *b_elozo;
        b_mozgo = *bullets;
        b_elozo = NULL;
        Asteroid temp;

        /*Végigmegyünk a lövedékek láncolt listáján*/
        while (b_mozgo != NULL) {

            /*Lövedék és hajó közötti táv*/
            d = sqrt(pow(b_mozgo->adat.poi.x - a_mozgo->adat.poi.x ,2) + pow(b_mozgo->adat.poi.y - a_mozgo->adat.poi.y, 2));

            //Ha a táv kissebb mint a vizsgált aszteroida mérete + a lövedék rádiusza, akkor eltalálta a lövedék az aszteroidát
            if(d < r + 5) {

                //Elpusztítjuk a vizsgált lövedéket
                DestroyBullet(&b_mozgo, bullets, &b_elozo);

                //Elmentjük a vizsgált aszteroida adatait
                temp = a_mozgo->adat;

                //Alkotunk egy robbanást a vizsgált aszteroida helyén
                Explode(temp, explosions);

                //Elpusztítjuk az aszteroidát
                DestroyAsteroid(&a_mozgo, asteroids, &a_elozo);

                /*Nagy aszteroida eseten Split(), ketto kicsi jelenik meg a helyen*/
                if(temp.size == 2)
                    Split(temp, asteroids);

                /*Kérdés: miért nem Split()-elünk először és utána DestroyAsteroid()-olunk (hogy ne keljen temp)?
                Ha a lista első eleme egy nagy aszteroida, és pont azt akarjuk kivenni, két új aszteroida fog a lista elejére kerülni,
                de a b_elozo változónk még NULL lesz, mivel ez az első kör a ciklusban. A DestroyAsteroid() azt fogja hinni, hogy a vizsgált elem
                az első a listában, ezért a másik kettő nem lesz belekötve, ami fatális memóriaszivárgást okoz. Ezért kell először temp-be menteni,
                DestroyAsteroid()-olni és utána Split()-elni*/

                //Minden (kicsi) elpusztított aszteroida után kapunk 50 pontot
                else *points = *points + 50;

                /*A DestroyAsteroid-ban benne van az is, hogy továbleétesse a mozgó pointert, ezért egy kapcsot csinálunk, hogy ne leptesse tovább ha ebben a ciklusban elpusztult*/
                destroy = true;

                /*Ez az aszteroida megsemmisült, nincs értelme a többi lövedékre rátesztelni, kilépünk a ciklusból*/
                break;
            }
            //Csinálunk egy lemaradót, és léptetjük a mozgót
            b_elozo = b_mozgo;
            b_mozgo = b_mozgo->kov;
        }

        //Ha a vizsgált aszteroida elpusztult ebben a ciklusban (ha destroy == true), akkor jelenleg a következő vizsgálandó elemen vagyunk, ezért kihagyjuk a léptetést
        if(!destroy) {
            a_elozo = a_mozgo;
            a_mozgo = a_mozgo->kov;
        }
        else destroy = false;

    }
    //Ha lement a ciklus, akkor a hajó még életben van. Ezért return false
    return false;
}

 /** \brief Felszabadítja a láncolt listáinkat. A játék végén van meghívva, hogy még a pályán maradt lefoglalt elemek ne okozzanak memóriaszivárgást
  *
  * \param  **asteroids: Az aszteroida lista első eleme
  * \param  **bullets: A lövedékek lista első eleme
  * \param  **explosions: A robbanások lista első eleme
  */
static void FreeAll(AsteroidList *asteroids, BulletList *bullets, ExplosionList *explosions) {

    //Végigfutunk a listán és töröljük az elemeit
    AsteroidList *a_mozgo;
    a_mozgo = asteroids;
    while (a_mozgo != NULL) {
        AsteroidList *a_kov = a_mozgo->kov;
        free(a_mozgo);
        a_mozgo = a_kov;
    }

    //Ditto
    BulletList *b_mozgo;
    b_mozgo = bullets;
    while (b_mozgo != NULL) {
        BulletList *b_kov = b_mozgo->kov;
        free(b_mozgo);
        b_mozgo = b_kov;
    }

    //Ditto
    ExplosionList *e_mozgo;
    e_mozgo = explosions;
    while (e_mozgo != NULL) {
        ExplosionList *e_kov = e_mozgo->kov;
        free(e_mozgo);
        e_mozgo = e_kov;
    }
}


bool game(SDL_Renderer *renderer, int dif, unsigned int *points) {

    //Alkotunk egy idő véletlenszerü változót
    srand(time(0));

    //A nehézség függvényében változik pár paraméter a játékon belül
    /*
    Könnyü:
    Esély az aszteroida megjelenésére: +0 (lásd: Asteroid_m())
    Pont minden képkockán: 1
    Aszteroida sebesség szorzó: 1
    Életek: +2
    */
    int chance_mod = 0;
    unsigned int score_mod = 1;
    float speed_mod = 1;
    int life_mod = 2;
    switch(dif) {
        /*
        Közepes:
        Esély az aszteroida megjelenésére: +5
        Pont minden képkockán: 2
        Aszteroida sebesség szorzó: 1.1
        Életek: +0
        */
        case 1:
            chance_mod = 5;
            score_mod = 2;
            speed_mod = 1.1;
            life_mod = 0;
            break;
        /*
        Nehéz:
        Esély az aszteroida megjelenésére: +10
        Pont minden képkockán: 2
        Aszteroida sebesség szorzó: 1.25
        Életek: +0
        */
        case 2:
            chance_mod = 10;
            score_mod = 2;
            speed_mod = 1.25;
            life_mod = 0;
            break;
        }


    //Inicializáljuk a játékost, a lenyomott billenttyűket, az életeket és a pontszámot
    Player player;
    Keys keys;
        keys.balra = keys.jobbra = keys.elore = keys.loves = keys.pause = false;
    int life = 2 + life_mod;
    *points = 0;

    //Megalkotjuk először a játékost
    player = Spawn(player);

    //Inicializálunk változókat, amely a játék logikályának megalkotásában segítenek minket

    bool death = false; //A játékos halott e, vagy sem
    int deathframe = 0; //A halál animációbol eltelt idő
    int game_state = 0; /*A játék állapota: 0 játék, 1 gameover, 2 szünet*/
    bool pause_buffer = false; /*Elősegít, hogy mikor lenyomjuk a szünet gombot, ne akarja a program egyböl folytatni a játékot, csak ha a billenttyűt felemeljük és megint lenyomjuk*/

    int bullet_grace = 0; // A következő lövésig hátralévő idő. Amíg nem 0, a játékos nem tud lőni

    BulletList *bullets; // A lövedék lista
        bullets = NULL;

    AsteroidList *asteroids; //Az aszteroida lista
        asteroids = NULL;

    ExplosionList *explosions; //Az robbanás lista
        explosions = NULL;

    bool shooting = false; //A játékos lő-e éppen (nem ugyanaz, mint a játékos lövés gombja, mivel lehet hogy az adott képkockán nem tud lőni még a játékos, de le van nyomva a gomb)
    char str_points[10]; // A képkockánkénti pontszám kiíráshoz szükséges változó, ez fogja fogadni a számból konvertált sztringet.

    int chance = 0; // Az esély arra, hogy megjelenjen egy aszteroida. Minden képkockán nő eggyel, vagy ha megjelenik egy aszteroida akkor nullázódik

    //Debug funkció: érzékeli le van e nyomva a "k" gomb. Ha igen, akkor egyböl Game Over-t kap a játék
    //bool killbind = false;

    SDL_Event ev; //Jelenlegi esemény

    SDL_TimerID id = SDL_AddTimer(20, idozit, NULL); //Időzítő, amely minden x miliszekundumban meghívja a SDL_USEREVENT eseményt, ezzel generálva az egyenletesen frissülő képkockákat. x = a függvény első paramétere

    //Ciklus, ami minden képkockán, vagy gomb lenyomásakor lefut. Kilép, ha a játékos meghalt, vagy ha be van nyomva a "k" (debug) gomb
    while (SDL_WaitEvent(&ev) && game_state != 1 /*&& !killbind*/) {

        switch(ev.type) {

            //Minden képkocka frissítéskor meghívott parancsok
            case SDL_USEREVENT:

                //Deaktiválja a pause_buffer-t, ha már nincs lenyomva a gomb. Fontos, hogy egy gomb lenyomásakor ne folytassa a játék magát egyböl, miután le lett állítva
                if(!keys.pause && pause_buffer) pause_buffer = false;

                //Ha nincs bufferelve és megnyomjuk a gombot
                if(!pause_buffer) {
                    if(keys.pause)  {

                        /*Pause*/
                        //Ha épp megy a játék, akkor álljon meg, és tegye át a game_state-et. Emiatt a kód nagy része sztázisba kerül, pl. a kép frissítése.
                        //Emiatt, a "Paused" szót elég egyszer kiírni a képernyő közepére
                        if(game_state == 0) {

                            //Aktiváljuk a buffert, így nem tud a játék még több gomblenyomást érzékelni egy gomblenyomás esetén
                            pause_buffer = true;
                            game_state = 2;
                            stringRGBA(renderer, 480, 360, "Paused", 255, 255, 255, 255);
                        }
                        /*Unpause*/
                        //Ha épp szünetelve van, tegye vissza a game_state változót, ezzel újra lefut a kód nagyrésze
                        else if(game_state == 2) {
                            pause_buffer = true;
                            game_state = 0;
                        }
                    }
                }

                //Ha nincs szüneteltetve
                if(game_state != 2) {

                    //Töröljünk mindent a képernyőről. Mindent megrajzolunk újra
                    boxRGBA(renderer, 0, 0, 960, 720, 0, 0, 0, 255);

                    //Ha halott a játékos
                    if(death) {

                        //Játsza le a halál effektust ezen a képkockán, valamint adjon egyet a halál idejéhez
                        DeathEffect(renderer, player.poi, deathframe);
                        deathframe++;

                        //Ha eléri a 100-at a halál ideje, akkor a játékos életeiböl levonunk egyett, újraélesztjük a játékost, újraállítjuk a halál idejét,
                        //és átkapcsoljuk a halott logikai változót
                        if(deathframe == 100) {

                            //Ha a játékosnak nem maradt több élete, akkor lépjünk ki a ciklusbol. Ezzel el fogjuk érni a modul végét, amely tudni fogja, hogy a játékos "Game Over"-t kapott
                            if(life <= 0) { game_state = 1; break; }
                            player = Spawn(player);
                            deathframe = 0;
                            death = false;
                            life--;
                        }
                    }
                    /*Innentől jönnek azok az események, amik csak akkor futnak le mikor a játékos él, és nincs szüneteltetve a játék*/
                    else {

                    //Balra vagy jobbra lenyomott billenttyű esetén, forgassuk 5 fokkal abba az irányba a játékost
                    if(keys.balra) {
                        player.rotation -= 5;

                    //Maradjon ez a szám 360 fok alatt, hogy ne alkossunk túl nagy számot
                    if(player.rotation >= 360) player.rotation -= 360;
                    }

                    //Kettő gomb lenyomása esetén a balra gomb prioritást élvez
                    else if(keys.jobbra) {
                        player.rotation += 5;

                    //Maradjon ez a szám 0 fok felett, hogy ne alkossunk túl negatív számot
                    if(player.rotation < 0) player.rotation += 360;
                    }

                    //Ha a játékos nyomva tartja a lövés gombot, és már eltelt a következő lövésig az idő, mondjuk meg, hogy lő éppen, és állítsuk be,
                    //hogy mennyi idő múlva tud újra lőni
                    if (keys.loves == true && bullet_grace == 0) {
                        shooting = true;
                        bullet_grace = 10;
                    }

                    //Számoljunk vissza az időböl, amíg nem érjük el az 0-át
                    if (bullet_grace > 0) bullet_grace -= 1;
                    else if (bullet_grace < 0) bullet_grace = 0;

                    //Frissítsük a játékos sebességét
                    player.dir = Move(player.dir, keys.elore, player.rotation);

                    //Új pozíció = régi + sebesség
                    player.poi.x = player.poi.x + player.dir.x;
                    player.poi.y = player.poi.y + player.dir.y;

                    //Teszteljük le az érintkezéseket a hajó, az aszteroidák, és a lovedékek között. Ha igazt ad vissza, a játékos nekiment egy aszteroidának, és meghalt
                    if(Collision(player.poi, &asteroids, &bullets, &explosions, player.inv, points)) death = true;

                    //Redukáljuk a játékos halhatatlanságából visszamaradó időt
                    if(player.inv > 0) player.inv -= 1;
                        else if(player.inv < 0) player.inv = 0;

                    //Játékos kirajzolása
                    DrawPlayer(renderer, player.poi.x, player.poi.y, player.rotation, player.inv);

                    //Növeljük a pontokat és mentsük le az összeget egy sztring változóba
                    *points = *points + score_mod;
                    sprintf(str_points, "%d", *points);
                    }

                    //Futtassuk a menedzsereket, hogy frissíthessük azoknak adatait és elemeit
                    bullets = Bullet_m(bullets, shooting, player.rotation, player.poi, player.dir);
                    asteroids = Asteroid_m(asteroids, bullets, &chance, chance_mod, speed_mod);
                    explosions = Explosion_m(renderer, explosions);

                    //Mivel következő képkockán amúgy sem tudna lőni, ezt újra
                    shooting = false;

                    //Futtassuk a rajzoló függvényeket a gameDraw.c modulból
                    Text_Create(renderer, str_points, 760, 56, 32, false);
                    DrawBullets(renderer, bullets);
                    DrawAsteroids(renderer, asteroids);
                    LifeUpdate(renderer, life);
                }

                //Kirajzoljuk az összes SDL rajzoló függvényt a képernyőre
                SDL_RenderPresent(renderer);
                break;

            //Gomblenyomások
            case SDL_KEYDOWN:
                switch(ev.key.keysym.sym) {
                    case SDLK_a: keys.balra = true; break;
                    case SDLK_d: keys.jobbra = true; break;
                    case SDLK_w: keys.elore = true; break;
                    case SDLK_SPACE: keys.loves = true; break;
                    case SDLK_p: keys.pause = true; break;
                    //case SDLK_k: killbind = true; break;
                } break;

            //Gombfelemelések
            case SDL_KEYUP:
                switch(ev.key.keysym.sym) {
                    case SDLK_a: keys.balra = false; break;
                    case SDLK_d: keys.jobbra = false; break;
                    case SDLK_w: keys.elore = false; break;
                    case SDLK_SPACE: keys.loves = false; break;
                    case SDLK_p: keys.pause = false; break;
                    //case SDLK_k: killbind = false; break;
                } break;

            //Az a jobb felso sarokban lévő "x" lenyomásakor fut le ez az esemény. Ekkor tudatjuk, hogy be kell zárni az egész ablakot,
            //jelezzük a függvénymeghívónak azzal, hogy true-t adunk vissza
            case SDL_QUIT:
                //Felszabadítjuk az összes listát, és leállítjuk az időzítőt
                FreeAll(asteroids, bullets, explosions);
                SDL_RemoveTimer(id);
                return true;

        }
    }
    //Felszabadítjuk az összes listát, és leállítjuk az időzítőt
    FreeAll(asteroids, bullets, explosions);
    SDL_RemoveTimer(id);
    //Ha itt tartunk, akkor a játékos kifogyott az életekből. Ekkor a játéknak vége de a programnak nem, ezért false-ot adunk vissza
    return false;
}


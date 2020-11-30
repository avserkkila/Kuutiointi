#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <listat.h>
#include <time.h>
#include "rakenteet.h"
#include "cfg.h"
#include "grafiikka.h"

int kaunnista(kaikki_s *kaikki);

/*alustaa grafiikan ja ikkunan ja renderin yms ja lataa fontin,
  käynnistää käyttöliittymän*/

int main(int argc, char** argv) {
  setlocale(LC_ALL, "fi_FI.utf8");
  kaikki_s kaikki;
  int r = 0;
  
  /*grafiikan alustaminen*/
  if (SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "Virhe: Ei voi alustaa SDL-grafiikkaa: %s\n", SDL_GetError());
    r = 1;
    goto EI_SDL;
  }
  if (TTF_Init()) {
    fprintf(stderr, "Virhe: Ei voi alustaa SDL_ttf-fonttikirjastoa: %s\n", TTF_GetError());
    SDL_Quit();
    r = 1;
    goto EI_TTF;
  }
  kaikki.ikkuna = SDL_CreateWindow\
    (ohjelman_nimi, ikkuna_x, ikkuna_y, ikkuna_w, ikkuna_h, SDL_WINDOW_RESIZABLE);
  kaikki.rend = SDL_CreateRenderer(kaikki.ikkuna, -1, SDL_RENDERER_TARGETTEXTURE);

  /*kello-olio*/
  tekstiolio_s kelloolio;
  kelloolio.teksti = malloc(90);
  strcpy(kelloolio.teksti, "");
  kelloolio.font = TTF_OpenFont(kellofonttied, kellokoko);
  if(!kelloolio.font) {
    fprintf(stderr, "Virhe: Ei avattu kellofonttia: %s\n", TTF_GetError());
    r = 1;
    goto EI_FONTTI;
  }
  kelloolio.sij = &kellosij;
  SDL_Rect kelloapu = (SDL_Rect){0, 0, 0, 0};
  kelloolio.toteutuma = &kelloapu;
  kelloolio.vari = kellovarit[0];
  kaikki.kello_o = &kelloolio;
  kaikki.kvarit = kellovarit;

  /*tulosolio*/
  tekstiolio_s tulosolio;
  tulosolio.font = TTF_OpenFont(tulosfonttied, tuloskoko);
  if(!tulosolio.font) {
    fprintf(stderr, "Virhe: Ei avattu tulosfonttia: %s\n", TTF_GetError());
    r = 1;
    goto EI_FONTTI;
  }
  tulosolio.sij = &tulossij;
  SDL_Rect tulosapu = (SDL_Rect){0, 0, 0, 0};
  tulosolio.toteutuma = &tulosapu;
  tulosolio.vari = tulosvari;
  tulosolio.rullaus = 0;
  tulosolio.numerointi = 1;
  kaikki.tulos_o = &tulosolio;

  /*jarjolio*/
  tekstiolio_s jarjolio;
  jarjolio.font = TTF_OpenFont(jarjfonttied, jarjkoko);
  if(!jarjolio.font) {
    fprintf(stderr, "Virhe: Ei avattu äärifonttia: %s\n", TTF_GetError());
    r = 1;
    goto EI_FONTTI;
  }
  jarjolio.sij = &jarjsij;
  SDL_Rect jarjapu = (SDL_Rect){0, 0, 0, 0};
  jarjolio.toteutuma = &jarjapu;
  jarjolio.vari = jarjvari;
  jarjolio.rullaus = 0;
  jarjolio.numerointi = 0;
  kaikki.jarj_o = &jarjolio;
  kaikki.jarjsuhde = jarjsuhde;


  /*tiedotolio*/
  tekstiolio_s tiedotolio;
  tiedotolio.font = TTF_OpenFont(tiedotfonttied, tiedotkoko);
  if(!tiedotolio.font) {
    fprintf(stderr, "Virhe: Ei avattu tiedotfonttia: %s\n", TTF_GetError());
    r = 1;
    goto EI_FONTTI;
  }
  tiedotolio.sij = &tiedotsij;
  SDL_Rect tiedotapu = (SDL_Rect){0, 0, 0, 0};
  tiedotolio.toteutuma = &tiedotapu;
  tiedotolio.vari = tiedotvari;
  tiedotolio.rullaus = 0;
  tiedotolio.numerointi = 0;
  kaikki.tiedot_o = &tiedotolio;

  /*tlukuolio on pitkälti sama kuin tiedotolio*/
  tekstiolio_s tlukuolio;
  tlukuolio.font = tiedotolio.font;
  tlukuolio.sij = &tluvutsij;
  SDL_Rect tlukuapu = (SDL_Rect){0, 0, 0, 0};
  tlukuolio.toteutuma = &tlukuapu;
  tlukuolio.vari = tiedotvari;
  tlukuolio.rullaus = 0;
  tlukuolio.numerointi = 0;
  kaikki.tluvut_o = &tlukuolio;

  /*lisätiedot*/
  tekstiolio_s lisaolio;
  lisaolio.font = TTF_OpenFont(lisafonttied, lisakoko);
  if(!lisaolio.font) {
    fprintf(stderr, "Virhe: Ei avattu lisafonttia: %s\n", TTF_GetError());
    r = 1;
    goto EI_FONTTI;
  }
  lisaolio.sij = &lisasij;
  SDL_Rect lisaapu = (SDL_Rect){0, 0, 0, 0};
  lisaolio.toteutuma = &lisaapu;
  lisaolio.vari = lisavari;
  lisaolio.rullaus = 0;
  lisaolio.numerointi = 0;
  kaikki.lisa_o = &lisaolio;

  /*sekoitusolio*/
  tekstiolio_s sektusolio;
  sektusolio.font = TTF_OpenFont(sektusfonttied, sektuskoko);
  if(!sektusolio.font) {
    fprintf(stderr, "Virhe: Ei avattu sekoitusfonttia: %s\n", TTF_GetError());
    r = 1;
    goto EI_FONTTI;
  }
  sektusolio.sij = &sektussij;
  SDL_Rect sektusapu = (SDL_Rect){0, 0, 0, 0};
  sektusolio.toteutuma = &sektusapu;
  sektusolio.vari = sektusvari;
  sektusolio.rullaus = 0;
  sektusolio.numerointi = 1;
  kaikki.sektus_o = &sektusolio;

  /*valintaolion teksti*/
  vnta_s vntaolio;
  vntaolio.valittu = 0;
  tekstiolio_s vto;
  vto.font = TTF_OpenFont(vntafonttied, vntakoko);
  if(!vto.font) {
    fprintf(stderr, "Virhe: Ei avattu valintafonttia: %s\n", TTF_GetError());
    r = 1;
    goto EI_FONTTI;
  }
  int rvali = TTF_FontLineSkip(vto.font);
  int d = (int)(rvali*1.2);
  int valipituus;
  TTF_GlyphMetrics(vto.font,' ',NULL,NULL,NULL,NULL,&valipituus);
  SDL_Rect ta = (SDL_Rect){vntasij.x + d + 3*valipituus,	\
			   vntasij.y + (d - rvali) / 2,	\
			   vntasij.w - d - 3*valipituus,	\
			   rvali};
  SDL_Rect vntaapu = (SDL_Rect){0, 0, 0, 0};
  vto.sij = &ta;
  vto.teksti = vntateksti;
  vto.toteutuma = &vntaapu;
  vto.vari = vntavari;
  vto.rullaus = 0;
  vto.numerointi = 0;
  vntaolio.teksti = &vto;
  
  /*valintaolion kuvat*/
  kuvarakenne kuvat;
  SDL_Surface* kuva;
  if(!(kuva = SDL_LoadBMP(url_valittu))) {
    fprintf(stderr, "Virhe: Ei luettu kuvaa \"%s\"\n", url_valittu);
    goto EI_KUVAA;
  }
  kuvat.valittu = SDL_CreateTextureFromSurface(kaikki.rend, kuva);
  SDL_FreeSurface(kuva);
  if(!(kuva = SDL_LoadBMP(url_eivalittu))) {
    fprintf(stderr, "Virhe: Ei luettu kuvaa \"%s\"\n", url_eivalittu);
    goto EI_KUVAA;
  }
  kuvat.ei_valittu = SDL_CreateTextureFromSurface(kaikki.rend, kuva);
  SDL_FreeSurface(kuva);
  SDL_Rect kuvaalue;
  kuvaalue.x = vntasij.x;
  kuvaalue.y = vntasij.y;
  kuvaalue.w = d;
  kuvaalue.h = d;
  kuvat.sij = &kuvaalue;
  vntaolio.kuvat = &kuvat;
  kaikki.vnta_o = &vntaolio;

  /*kiinnittämättömät*/
  laitot_s laitot = (laitot_s){1, 1, 1, 1, 1, 1, 1};
  kaikki.laitot = &laitot;
  kaikki.viive = viive;
  kaikki.strtulos = NULL;
  kaikki.liukutulos = NULL;
  kaikki.tietoalut = _yalkuun(_strlistaksi(tietoalkustr, "|"));
  kaikki.sekoitukset = NULL;
  kaikki.tiedot = NULL;
  kaikki.lisatd = NULL;
  kaikki.sjarj = NULL;
  kaikki.sijarj = NULL;
  kaikki.fjarj = NULL;
  kaikki.sjarj = _strlisaa_kopioiden(kaikki.sjarj, "");
  kaikki.sijarj = _strlisaa_kopioiden(kaikki.sijarj, "");
  kaikki.fjarj = _flisaa(kaikki.fjarj, -INFINITY); //alkuun ylimääräinen

  time_t t;
  srand((unsigned) time(&t));
  strcpy(kaikki.kello_o->teksti, " ");

  SDL_RenderClear(kaikki.rend);
  SDL_SetRenderDrawColor(kaikki.rend, 0, 0, 0, 255);
  r = kaunnista(&kaikki);
  //tähän tulosten kirjaaminen

  SDL_DestroyTexture(kaikki.vnta_o->kuvat->valittu);
  SDL_DestroyTexture(kaikki.vnta_o->kuvat->ei_valittu);
  _strpoista_kaikki(_yalkuun(kaikki.strtulos));
  _strpoista_kaikki(_yalkuun(kaikki.tietoalut));
  _strpoista_kaikki(_yalkuun(kaikki.sekoitukset));
  _strpoista_kaikki(_yalkuun(kaikki.tiedot));
  _yrma(_yalkuun(kaikki.liukutulos));
  free(kelloolio.teksti);
 EI_KUVAA:
  TTF_CloseFont(kelloolio.font);
  TTF_CloseFont(tulosolio.font);
  TTF_CloseFont(jarjolio.font);
  TTF_CloseFont(tiedotolio.font);
  TTF_CloseFont(sektusolio.font);
  TTF_CloseFont(vto.font);
  TTF_CloseFont(lisaolio.font);
 EI_FONTTI:
  SDL_DestroyRenderer(kaikki.rend);
  SDL_DestroyWindow(kaikki.ikkuna);
  TTF_Quit();
 EI_TTF:
  SDL_Quit();
 EI_SDL:
  return r;
}

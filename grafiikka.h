#ifndef __grafiikka__
#define __grafiikka__

#include <SDL2/SDL_ttf.h>
#include "listat.h"

typedef struct {
  char* teksti;
  char ttflaji; //mitä ttf-kirjaston funktiota käytetään (katso laita_teksti_ttf())
  TTF_Font* font;
  int fonttikoko;
  char* fonttied;
  SDL_Rect sij; //tälle varattu maksimitila
  SDL_Rect toteutuma; //mikä tila oikeasti käytetään
  SDL_Color vari;
  short alku; //koskee vain listoja, ensimmäisen näytetyn indeksi, 'r'
  short rullaus; //koskee vain listoja, 'w'
  char numerointi; //koskee vain listoja;
} tekstiolio_s;

typedef struct {
  SDL_Texture* valittu;
  SDL_Texture* ei_valittu;
  SDL_Rect sij;
} kuvarakenne;

typedef struct {
  tekstiolio_s teksti;
  kuvarakenne kuvat;
  char valittu;
} vnta_s;

typedef struct {
  SDL_Rect kulmio;
  int paksuus;
  SDL_Color vari;
} korostustietue;

#endif

extern unsigned short laitot;
extern const unsigned short kellolai;
extern const unsigned short vntalai;
extern const unsigned short sektuslai;
extern const unsigned short tuloslai;
extern const unsigned short jarjlai;
extern const unsigned short tiedtlai;
extern const unsigned short lisatdlai;
extern const unsigned short muutlai;
extern const unsigned short tkstallai;
extern const unsigned short korostuslai;
extern const unsigned short kaikki_laitot;
extern const unsigned short jaaduta;

void aseta_vari(SDL_Renderer*, SDL_Color*);
int xsijainti(tekstiolio_s* ol, int p);
void saada_kohdistin();
void piirrä();
void laita_teksti_ttf(tekstiolio_s*, SDL_Renderer*);
int laita_tekstilista(slista*, int, tekstiolio_s*, SDL_Renderer*);
void laita_jarjlista();
void laita_ehkä_korostus();
void laita_valinta(vnta_s* o, SDL_Renderer *rend);
void laita_oikealle(tekstiolio_s* ov, short vali, slista* l, int alku, tekstiolio_s* o, SDL_Renderer* r);
void laita_teksti_ttf_vasemmalle(tekstiolio_s* ov, short vali, tekstiolio_s* o, SDL_Renderer* r);

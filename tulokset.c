#include <lista_math.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "tulokset.h"
#include "rakenteet.h"

avgtulos avgn(flista* l, int n, int pois) {
  avgtulos r;
  flista *ml = _yalkuun(floatmovavg(_yalkuun(l), n-1, 0, pois, pois, -1));
  r.nyt = (ml)? ((flista*)_yloppuun(ml))->f : NAN;

  /*maks ja min*/
  floatint tmp = floatmax(ml, -1);
  r.max = tmp.a;
  r.maxind = tmp.b;
  tmp = floatmin(ml, -1);
  r.min = tmp.a;
  r.minind = tmp.b;
  _yrma(ml);
  return r;
}

double sigma(flista* fl, int n, int pois) {
  flista* l = _yalkuun(_fkopioi(fl, n));
  if(_ylaske(l) != n)
    return NAN;
  flista* alku = l;
  floatjarjesta(l, floatmin, NULL, -1);
  for(int i=0; i<pois && l; i++)
    l = l->seur;
  double r = floatstd(l, n-(pois*2));
  _yrma(alku);
  return r;
}

strlista* tee_tiedot(strlista* strtiedot, flista* fl, int* avgind) {
  avgtulos at;
  char *tmp;
  tmp = calloc(200, 1);
  _strpoista_kaikki(_yalkuun(strtiedot));
  strtiedot = NULL;

  /*Avg5*/
  at = avgn(fl, 5, 1);
  sprintf(tmp, " = %.2f  (%.2f – %.2f)", at.nyt, at.min, at.max);
  strtiedot = _strlisaa_kopioiden(strtiedot, tmp);
  double a, b, c;
  a = sigma(_ynouda(fl, -4), 5, 1);
  b = sigma(_ynouda(_yalkuun(fl), at.minind-4), 5, 1);
  c = sigma(_ynouda(_yalkuun(fl), at.maxind-4), 5, 1);
  sprintf(tmp, " = %.2lf   %.2lf   %.2lf", a, b, c);
  strtiedot = _strlisaa_kopioiden(strtiedot, tmp);
  if(avgind) {
    avgind[0] = _ylaske(_yalkuun(fl))-1;
    avgind[1] = at.minind;
    avgind[2] = at.maxind;
    avgind += 3;
  }

  /*Avg12*/
  at = avgn(fl, 12, 1);
  sprintf(tmp, " = %.2f  (%.2f – %.2f)", at.nyt, at.min, at.max);
  strtiedot = _strlisaa_kopioiden(strtiedot, tmp);
  a = sigma(_ynouda(fl, -11), 12, 1);
  b = sigma(_ynouda(_yalkuun(fl), at.minind-11), 12, 1);
  c = sigma(_ynouda(_yalkuun(fl), at.maxind-11), 12, 1);
  sprintf(tmp, " = %.2lf   %.2lf   %.2lf", a, b, c);
  strtiedot = _strlisaa_kopioiden(strtiedot, tmp);
  if(avgind) {
    avgind[0] = _ylaske(_yalkuun(fl))-1;
    avgind[1] = at.minind;
    avgind[2] = at.maxind;
  }

  /*Keskiarvo*/
  sprintf(tmp, " = %.2lf (σ = %.2f)",		\
	  floatmean(_yalkuun(fl), -1), floatstd(_yalkuun(fl), -1));
  strtiedot = _strlisaa_kopioiden(strtiedot, tmp);

  /*Mediaani*/
  floatint med = floatmed(_yalkuun(fl), -1);
  sprintf(tmp, " = %.2f (%i.)", med.a, med.b+1);
  strtiedot = _strlisaa_kopioiden(strtiedot, tmp);
  free(tmp);
  return strtiedot;
}

/*laittaa yhden suurimman ja pienimmän sulkeisiin*/
strlista* tee_lisatiedot(strlista* sl, flista* fl, strlista* sektus, int alkuind, int n) {
  strlista* r = NULL;
  char tmp[100];
  char* apucp;
  sl = _ynouda(_yalkuun(sl), alkuind);
  fl = _ynouda(_yalkuun(fl), alkuind);
  if(sektus) sektus = _ynouda(_yalkuun(sektus), alkuind);
  if(!sl)
    return NULL;
  sprintf(tmp, "Avg%i: %.2f; σ = %.2f", n, floatavg(fl, 0, n-1, 1, 1), sigma(fl, n, 1));
  while( (apucp = strstr(tmp, ".")) )
    *apucp = ',';
  r = _strlisaa_kopioiden(r, tmp);
  floatint max = floatmax(fl, n);
  floatint min = floatmin(fl, n);
  for(int i=0; i<n; i++) {
    if(i == max.b || i == min.b)
      sprintf(tmp, "(%i. %s)", i+1+alkuind, sl->str);
    else
      sprintf(tmp, " %i. %s", i+1+alkuind, sl->str);
    if(sektus) {
      sprintf(tmp, "%s   %s", tmp, sektus->str);
      sektus = sektus->seur;
    }
    r = _strlisaa_kopioiden(r, tmp);
    sl = sl->seur;
  }
  return _yalkuun(r);
}

/*mille paikalle f laitetaan listassa, joka on järjestetty pienimmästä suurimpaan*/
int hae_paikka(float f, flista* l) {
  if(!isfinite(f))
    return 0x0fffffff; //ei laiteta tähän DNF:iä
  int paikka = 0;
  while(l && f > l->f) {
    l = l->seur;
    paikka++;
  }
  return paikka;
}

int hae_silistalta(strlista* l, int i) {
  char tmp[10];
  sprintf(tmp, "%i. ", i);
  int r = 0;
  while(l) {
    if(!strcmp(l->str, tmp))
      return r;
    r++;
    l = l->seur;
  }
  return -1;
}

/*i on tuloksen indeksi nollasta alkaen järjestämättä*/
void poista_jarjlistalta(int i, strlista** si, strlista** s, flista** fl) {
  int paikka = hae_silistalta(_yalkuun(*si), i+1);
  if(paikka < 0)
    goto NUMEROINTI; //kyseistä ei ollut jarjlistalla ensinkään
  char palsuunta = (paikka == 0)? 1 : -1;
  *si = _strpoista1(_ynouda(_yalkuun(*si), paikka), palsuunta);
  *s  = _strpoista1(_ynouda(_yalkuun(*s ), paikka), palsuunta);
  *fl = _yrm1(_ynouda(_yalkuun(*fl), paikka), palsuunta);

 NUMEROINTI:;
  /*korjataan numerointi, jos poistettiin välistä*/
  /*haetaan ensin maksimi si-listalta*/
  strlista *l = _yalkuun(*si);
  int maks = 0;
  int yrite = 0;
  while(l) {
    sscanf(l->str, "%i", &yrite);
    if(yrite > maks)
      maks = yrite;
    l = l->seur;
  }

  /*sitten korjataan*/
  l = _yalkuun(*si);
  strlista* apu;
  char tmpc[10];
  for(int luku = i+1; luku<=maks; luku++) {
    paikka = hae_silistalta(l, luku);
    if(paikka<0)
      continue;
    sprintf(tmpc, "%i. ", luku-1);
    apu = _ynouda(l, paikka);
    if(*si == apu)
      *si = apu->edel;
    apu = _strpoista1(apu, -1);
    _strlisaa_kopioiden(apu, tmpc);
  }
}

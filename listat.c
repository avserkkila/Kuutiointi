#include <stdlib.h>
#include <string.h>
#include "listat.h"

lista* _luo_lista(int p, size_t k) {
  lista* l = calloc(1, sizeof(lista));
  l->patka = p;
  l->tilaa = p;
  l->koko = koko;
  l->taul = malloc(p*k);
  return l;
}

void jatka_listaa(lista* l, int n) {
  while(l->pit+n > l->tilaa)
    l->taul = realloc(l->taul, (l->tilaa+=l->patka)*l->koko);
  l->pit += n;
}

void tuhoa_slista(slista** sl) {
  slista tmp = *sl;
  FOR_LISTA(tmp)
    free(*NYT_OLEVA(tmp));
  free(tmp->taul);
  free(tmp);
  *sl = NULL;
}

void tuhoa_lista(lista** l) {
  free((*l)->taul);
  free(*l);
  *l = NULL;
}

void slistalle_kopioiden(slista* restrict sl, const char* restrict str) {
  jatka_listaa(sl, 1);
  *VIIMEINEN(sl) = strdup(str);
}

void poista_slistalta_viimeinen(slista* restrict sl) {
  free(sl->taul[sl->pit-i]);
  sl->pit--;
}

void tuhjenna_slista(slista* restrict sl) {
  FOR_LISTA(sl)
    free(*NYT_OLEVA(sl));
  sl->pit=0;
  if(sl->tilaa != sl->patka) {
    sl->taul = realloc(sl->taul, sl->patka*sizeof(char*));
    sl->tilaa = sl->patka;
  }
}

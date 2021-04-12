#include <stdio.h>
#include <SDL.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include "kuutio.h"
#include "kuution_grafiikka.h"
#include "muistin_jako.h"
#include "kuution_kommunikointi.h"
#include "python_savel.h"

#define PI 3.14159265358979

kuutio_t kuutio;
kuva_t kuva;
int3 akst[6];
#ifdef __KUUTION_KOMMUNIKOINTI__
int viimeViesti;
shmRak_s *ipc;
volatile float* savelPtr;
#endif

void seis() { //tarvitaan virheenjäljitykseen (gdb: break seis)
  ;
}

kuutio_t luo_kuutio(const unsigned char N) {
  vari varit[6];
  varit[_u] = VARI(255,255,255); //valkoinen
  varit[_f] = VARI(0,  220,  0); //vihreä
  varit[_r] = VARI(255,0  ,0  ); //punainen
  varit[_d] = VARI(255,255,0  ); //keltainen
  varit[_b] = VARI(0,  0,  255); //sininen
  varit[_l] = VARI(220,120,0  ); //oranssi
  
  kuutio.N = N;
  kuutio.xyz = (koordf){{PI/6, -PI/6, 0}};
  kuutio.sivut = malloc(6*sizeof(char*));
  kuutio.ruudut = malloc(6*4*kuutio.N*kuutio.N*sizeof(koordf));
  kuutio.varit = malloc(sizeof(varit));
  kuutio.ratkaistu = 1;
  
  for(int i=0; i<6; i++) {
    kuutio.varit[i] = varit[i];
    kuutio.sivut[i] = malloc(N*N);
    for(int j=0; j<N*N; j++)
      kuutio.sivut[i][j] = i;
  }
  tee_ruutujen_koordtit();
  return kuutio;
}

void tuhoa_kuutio() {
  for(int i=0; i<6; i++) {
    free(kuutio.sivut[i]);
    kuutio.sivut[i] = NULL;
  }
  free(kuutio.sivut);
  free(kuutio.ruudut);
  kuutio.ruudut = NULL;
  kuutio.sivut = NULL;
  free(kuutio.varit);
  kuutio.varit = NULL;
}

void paivita() {
  kuva.paivita = 0;
  SDL_SetRenderDrawColor(kuva.rend, 0, 0, 0, 255);
  SDL_RenderClear(kuva.rend);
  piirra_kuvaksi();
  if(kuva.korostus >= 0)
    korosta_tahko(kuva.korostus);
  if(kuva.ruutuKorostus.a[0] >= 0)
    korosta_ruutu(kuutio.ruudut+RUUTUINT3(kuva.ruutuKorostus), 3);
  SDL_RenderPresent(kuva.rend);
}

/*esim _u, 3, 0 (3x3x3-kuutio) --> _r, 2, 0:
  palauttaa oikean ruudun koordinaatit, kun yksi indeksi menee alunperin yhden yli tahkolta*/
int3 hae_ruutu(int tahko0, int i0, int j0) {
  int aksTahko0;
  for(aksTahko0=0; aksTahko0<3; aksTahko0++)
    if(ABS(akst[tahko0].a[aksTahko0]) == 3)
      break;
  if((i0 < 0 || i0 >= kuutio.N) && (j0 < 0 || j0 >= kuutio.N))
    return (int3){{-1, -1, -1}};
  int IvaiJ = (i0<0)? -1: (i0>=kuutio.N)? 1: (j0<0)? -2: (j0>=kuutio.N)? 2: 0;
  if(!IvaiJ)
    return (int3){{tahko0, i0, j0}};
  int ylimenoaks;
  for(ylimenoaks=0; ylimenoaks<3; ylimenoaks++)
    if(ABS(akst[tahko0].a[ylimenoaks]) == ABS(IvaiJ)) //akseli, jolta kys tahko menee yli
      break;
  int tahko1;
  int haluttu = 3*SIGN(IvaiJ)*SIGN(akst[tahko0].a[ylimenoaks]); //menosuunta ja lukusuunta
  for(tahko1=0; tahko1<6; tahko1++)
    if(akst[tahko1].a[ylimenoaks] == haluttu)
      break;
  int ij1[2], ind;
  /*tullaanko alkuun vai loppuun eli kasvaako uusi indeksi vanhan tahkon sijainnin suuntaan
    myös, että tullaanko i- vai j-akselin suunnasta*/
  int tulo = akst[tahko1].a[aksTahko0] * SIGN(akst[tahko0].a[aksTahko0]);
  ind = ABS(tulo)-1; //±1->0->i, ±2->1->j
  ij1[ind] = (tulo<0)? 0: kuutio.N-1; //negat --> tullaan negatiiviselta suunnalta
  ind = (ind+1)%2;
  /*toisen indeksin akseli on molempiin tahkoakseleihin kohtisuorassa*/
  int akseli2 = 3-aksTahko0-ABS(ylimenoaks);
  ij1[ind] = (ABS(akst[tahko0].a[akseli2]) == 1)? i0: j0;
  /*vaihdetaan, jos ovat vastakkaissuuntaiset*/
  if(akst[tahko0].a[akseli2] * akst[tahko1].a[akseli2] < 0)
    ij1[ind] = kuutio.N-1 - ij1[ind];
  return (int3){{tahko1, ij1[0], ij1[1]}};
}

void siirto(int puoli, char siirtokaista, char maara) {
  if(maara == 0)
    return;
  char N = kuutio.N;
  if(siirtokaista < 1 || siirtokaista > N)
    return;
  else if(siirtokaista == N) {
    maara = (maara+2) % 4;
    puoli = (puoli+3) % 6;
    siirtokaista = 1;
  }
  struct i4 {int i[4];} jarj;
  struct i4 kaistat;
  int a,b; //kaistat kahdelta eri puolelta katsottuna
  int kaista1,kaista2;
  char apu[N*N];
  char *sivu1, *sivu2;

  a = N-siirtokaista;
  b = (N-1)-a;
  
  /*toiminta jaetaan kääntöakselin perusteella*/
  switch(puoli) {
  case _r:
  case _l:
    if(puoli == _r) {
      kaistat = (struct i4){{a,a,a,b}};
      jarj = (struct i4){{_u, _f, _d, _b}};
    } else {
      jarj = (struct i4){{_u, _b, _d, _f}};
      kaistat = (struct i4){{b,a,b,b}};
    }
    sivu1 = kuutio.sivut[jarj.i[0]];
    /*1. sivu talteen*/
    for(int i=0; i<N; i++)
      apu[i] = sivu1[N*kaistat.i[0]+i];
    /*siirretään*/
    for(int j=0; j<3; j++) {
      sivu1 = kuutio.sivut[jarj.i[j]];
      sivu2 = kuutio.sivut[jarj.i[j+1]];
      kaista1 = kaistat.i[j];
      kaista2 = kaistat.i[j+1];
      if( (j<2 && puoli == _l) || (j>=2 && puoli == _r) )
	for(int i=0; i<N; i++)
	  sivu1[N*kaista1+i] = sivu2[N*kaista2 + N-1-i];
      else
	for(int i=0; i<N; i++)
	  sivu1[N*kaista1+i] = sivu2[N*kaista2+i];
    }
    /*viimeinen*/
    sivu1 = kuutio.sivut[jarj.i[3]];
    if(puoli == _r)
      for(int i=0; i<N; i++)
	sivu1[N*kaistat.i[3]+i] = apu[N-1-i];
    else
      for(int i=0; i<N; i++)
	sivu1[N*kaistat.i[3]+i] = apu[i];
    break;

  case _d:
  case _u:
    if(puoli == _d) {
      kaistat = (struct i4){{a,a,a,a}};
      jarj = (struct i4){{_f, _l, _b, _r}};
    } else {
      jarj = (struct i4){{_f, _r, _b, _l}};
      kaistat = (struct i4){{b,b,b,b}};
    }
    sivu1 = kuutio.sivut[jarj.i[0]];
    /*1. sivu talteen*/
    for(int i=0; i<N; i++)
      apu[i] = sivu1[N*i + kaistat.i[0]];
    /*siirretään*/
    for(int j=0; j<3; j++) {
      sivu1 = kuutio.sivut[jarj.i[j]];
      sivu2 = kuutio.sivut[jarj.i[j+1]];
      kaista1 = kaistat.i[j];
      kaista2 = kaistat.i[j+1];
      for(int i=0; i<N; i++)
	sivu1[N*i + kaista1] = sivu2[N*i + kaista2];
    }
    /*viimeinen*/
    sivu1 = kuutio.sivut[jarj.i[3]];
    for(int i=0; i<N; i++)
      sivu1[N*i+kaistat.i[3]] = apu[i];
    break;

  case _f:
  case _b:
    if(puoli == _f) {
      kaistat = (struct i4){{a,a,b,b}};
      jarj = (struct i4){{_u, _l, _d, _r}};
    } else {
      kaistat = (struct i4){{b,a,a,b}};
      jarj = (struct i4){{_u, _r, _d, _l}};
    }
    sivu1 = kuutio.sivut[jarj.i[0]];
    /*1. sivu talteen*/
    for(int i=0; i<N; i++)
      apu[i] = sivu1[N*i + kaistat.i[0]];
    /*siirretään, tässä vuorottelee, mennäänkö i- vai j-suunnassa*/
    for(int j=0; j<3; j++) {
      sivu1 = kuutio.sivut[jarj.i[j]];
      sivu2 = kuutio.sivut[jarj.i[j+1]];
      kaista1 = kaistat.i[j];
      kaista2 = kaistat.i[j+1];
      if(j % 2 == 0) //u tai d alussa
	if(puoli == _f) {
	  for(int i=0; i<N; i++) //1: i-suunta, 2: j-suunta
	    sivu1[N*i + kaista1] = sivu2[N*kaista2 + N-1-i];
	} else {
	  for(int i=0; i<N; i++) //1: i-suunta, 2: j-suunta
	    sivu1[N*i + kaista1] = sivu2[N*kaista2 + i];
	}
      else
	if(puoli == _b) {
	  for(int i=0; i<N; i++) //1: j-suunta, 2:i-suunta
	    sivu1[N*kaista1 + i] = sivu2[N*(N-1-i) + kaista2];
	} else {
	  for(int i=0; i<N; i++) //1: j-suunta, 2:i-suunta
	    sivu1[N*kaista1 + i] = sivu2[N*i + kaista2];
	}
    }
    /*viimeinen*/
    sivu1 = kuutio.sivut[jarj.i[3]];
    if(puoli == _f)
      for(int i=0; i<N; i++)
	sivu1[N*kaistat.i[3]+i] = apu[i];
    else
      for(int i=0; i<N; i++)
	sivu1[N*kaistat.i[3]+i] = apu[N-1-i];
    break;
  default:
    break;
  }

  /*siivusiirrolle (slice move) kääntö on nyt suoritettu*/
  if(siirtokaista > 1 && siirtokaista < N) {
    siirto(puoli, siirtokaista, maara-1);
    return;
  }
  
  /*käännetään käännetty sivu*/
  int sivuInd;
  switch(puoli) {
  case _r:
    sivuInd = _r;
    if(siirtokaista == N)
      sivuInd = _l;
    break;
  case _l:
    sivuInd = _l;
    if(siirtokaista == N)
      sivuInd = _r;
    break;
  case _u:
    sivuInd = _u;
    if(siirtokaista == N)
      sivuInd = _d;
    break;
  case _d:
    sivuInd = _d;
    if(siirtokaista == N)
      sivuInd = _u;
    break;
  case _b:
    sivuInd = _b;
    if(siirtokaista == N)
      sivuInd = _f;
    break;
  case _f:
    sivuInd = _f;
    if(siirtokaista == N)
      sivuInd = _b;
    break;
  default:
    return;
  }
  char* sivu = kuutio.sivut[sivuInd];
  /*kopioidaan kys. sivu ensin*/
  for(int i=0; i<N*N; i++)
    apu[i] = sivu[i];

  /*nyt käännetään: */
#define arvo(sivu,j,i) sivu[i*N+j]
  for(int i=0; i<N; i++)
    for(int j=0; j<N; j++)
      arvo(sivu,j,i) = arvo(apu,N-1-i,j);
#undef arvo
  siirto(puoli, siirtokaista, maara-1);
}

void kaanto(char akseli, char maara) {
  if(!maara)
    return;
  if(maara < 0)
    maara += 4;
  char sivu;
  switch(akseli) {
  case 'x':
    sivu = _r;
    break;
  case 'y':
    sivu = _u;
    break;
  case 'z':
    sivu = _f;
    break;
  default:
    return;
  }
  for(char i=1; i<=kuutio.N; i++)
    siirto(sivu, i, 1);
  
  kaanto(akseli, maara-1);
  return;
}

inline char __attribute__((always_inline)) onkoRatkaistu() {
  for(int sivu=0; sivu<6; sivu++) {
    char laji = kuutio.sivut[sivu][0];
    for(int i=1; i<kuutio.N*kuutio.N; i++)
      if(kuutio.sivut[sivu][i] != laji)
	return 0;
  }
  return 1;
}

char kontrol = 0;
float kaantoaika;
float kaantoaika0 = 0.2;
double viimeKaantohetki = -1.0;

inline void __attribute__((always_inline)) kaantoInl(char akseli, char maara) {
  kaanto(akseli, maara);
  kuva.paivita = 1;
}

inline void __attribute__((always_inline)) siirtoInl(int tahko, char kaista, char maara) {
  if(kontrol)
    return;
  struct timeval hetki;
  gettimeofday(&hetki, NULL);
  double hetkiNyt = hetki.tv_sec + hetki.tv_usec*1.0/1000000;
  if(viimeKaantohetki > 0) {
    double erotus = hetkiNyt - viimeKaantohetki;
    if(erotus < kaantoaika0)
      kaantoaika = erotus;
    else
      kaantoaika = kaantoaika0;
  }
  koordf akseli = (koordf){{(akst[tahko].a[0]/3),	\
			    (akst[tahko].a[1]/3),	\
			    (akst[tahko].a[2]/3)}};
  kaantoanimaatio(tahko, kaista, akseli, maara-2, kaantoaika);
  tee_ruutujen_koordtit();
  siirto(tahko, kaista, maara);
  kuva.paivita = 1;
  kuutio.ratkaistu = onkoRatkaistu();
#ifdef __KUUTION_KOMMUNIKOINTI__
  if(viimeViesti == ipcTarkastelu) {
    ipc->viesti = ipcAloita;
    viimeViesti = ipcAloita;
  } else if(viimeViesti == ipcAloita && kuutio.ratkaistu) {
    ipc->viesti = ipcLopeta;
    viimeViesti = ipcLopeta;
  }
#endif
  gettimeofday(&hetki, NULL);
  viimeKaantohetki = hetki.tv_sec + hetki.tv_usec*1.0/1000000;
}

int main(int argc, char** argv) {
  char ohjelman_nimi[] = "Kuutio";
  int ikkuna_x = 300;
  int ikkuna_y = 300;
  int ikkuna_w = 500;
  int ikkuna_h = 500;
  char N;
  char oli_sdl = 0;
  if(argc < 2)
    N = 3;
  else
    sscanf(argv[1], "%hhu", &N);
  
  if(!SDL_WasInit(SDL_INIT_VIDEO))
    SDL_Init(SDL_INIT_VIDEO);
  else
    oli_sdl = 1;

  /*Kuvan tekeminen*/
  kuva.ikkuna = SDL_CreateWindow\
    (ohjelman_nimi, ikkuna_x, ikkuna_y, ikkuna_w, ikkuna_h, SDL_WINDOW_RESIZABLE);
  kuva.rend = SDL_CreateRenderer(kuva.ikkuna, -1, SDL_RENDERER_TARGETTEXTURE);
  if(!kuva.rend)
    goto ULOS;
  kuva.xRes = ikkuna_w;
  kuva.yRes = ikkuna_h;
  kuva.mustaOsuus = 0.05;
  kuva.paivita = 1;
  kuva.korostus = -1;
  kuva.ruutuKorostus = (int3){{-1, kuutio.N/2, kuutio.N/2}};
  kuva.korostusVari = VARI(80, 233, 166);
  kuva.resKuut = (ikkuna_h < ikkuna_w)? ikkuna_h/sqrt(3.0)/2 : ikkuna_w/sqrt(3.0);
  kuva.sij0 = (ikkuna_h < ikkuna_w)? (ikkuna_h-kuva.resKuut)/2: (ikkuna_w-kuva.resKuut)/2;

  kuutio = luo_kuutio(N);
  kaantoaika = kaantoaika0;

  /*akselit*/
  /*esim. oikealla (r) j liikuttaa negatiiviseen y-suuntaan (1.indeksi = y, ±2 = j)
  i liikuttaa negatiiviseen z-suuntaan (2. indeksi = z, ±1 = i)
  sijainti on positiivisella x-akselilla (0. indeksi = x, ±3 = sijainti)*/
  akst[_r] =  (int3){{3, -2, -1}};
  akst[_l] = (int3){{-3, -2, 1}};
  akst[_u] = (int3){{1, 3, 2}};
  akst[_d] = (int3){{1, -3, -2}};
  akst[_f] = (int3){{1, -2, 3}};
  akst[_b] = (int3){{-1, -2, -3}};
  
#ifdef __KUUTION_KOMMUNIKOINTI__
  ipc = liity_muistiin();
  if(!ipc)
    return 1;
#endif

#define siirtoInl1(tahko, maara) siirtoInl(tahko, siirtokaista, maara)
  
  SDL_Event tapaht;
  int xVanha, yVanha;
  char hiiri_painettu = 0;
  char siirtokaista = 1;
  while(1) {
    while(SDL_PollEvent(&tapaht)) {
      switch(tapaht.type) {
      case SDL_QUIT:
	goto ULOS;
      case SDL_WINDOWEVENT:
	switch(tapaht.window.event) {
	case SDL_WINDOWEVENT_RESIZED:;
	  int koko1 = (tapaht.window.data1 < tapaht.window.data2)? tapaht.window.data1: tapaht.window.data2;
	  kuva.resKuut = koko1/sqrt(3.0);
	  if((koko1-kuva.resKuut)/2 != kuva.sij0) {
	    kuva.sij0 = (koko1-kuva.resKuut)/2;
	    tee_ruutujen_koordtit();
	    kuva.paivita = 1;
	  }
	  break;
	}
	break;
      case SDL_KEYDOWN:
	switch(tapaht.key.keysym.scancode) {
	case SDL_SCANCODE_I:
	  siirtoInl1(_u, 1);
	  break;
	case SDL_SCANCODE_L:
	  siirtoInl1(_r, 1);
	  break;
	case SDL_SCANCODE_J:
	  siirtoInl1(_r, 3);
	  break;
	case SDL_SCANCODE_PERIOD:
	  siirtoInl1(_d, 3);
	  break;
	case SDL_SCANCODE_K:
	  siirtoInl1(_f, 1);
	  break;
	case SDL_SCANCODE_O:
	  siirtoInl1(_b, 3);
	  break;
	case SDL_SCANCODE_E:
	  siirtoInl1(_u, 3);
	  break;
	case SDL_SCANCODE_F:
	  siirtoInl1(_l, 1);
	  break;
	case SDL_SCANCODE_S:
	  siirtoInl1(_l, 3);
	  break;
	case SDL_SCANCODE_X:
	  siirtoInl1(_d, 1);
	  break;
	case SDL_SCANCODE_D:
	  siirtoInl1(_f, 3);
	  break;
	case SDL_SCANCODE_W:
	  siirtoInl1(_b, 1);
	  break;
	case SDL_SCANCODE_U:
	  for(int kaista=2; kaista<N; kaista++)
	    siirtoInl(_r, kaista, 1);
	  break;
	case SDL_SCANCODE_R:
	  for(int kaista=2; kaista<N; kaista++)
	    siirtoInl(_l, kaista, 1);
	  break;
	  /*käytetään kääntämisten määrässä siirtokaistaa*/
	case SDL_SCANCODE_H:
	  kaantoInl('y', (3*siirtokaista) % 4);
	  break;
	case SDL_SCANCODE_G:
	  kaantoInl('y', (1*siirtokaista) % 4);
	  break;
	case SDL_SCANCODE_N:
	  kaantoInl('x', (1*siirtokaista) % 4);
	  break;
	case SDL_SCANCODE_V:
	  kaantoInl('x', (3*siirtokaista) % 4);
	  break;
	case SDL_SCANCODE_COMMA:
	  kaantoInl('z', (1*siirtokaista) % 4);
	  break;
	case SDL_SCANCODE_C:
	  kaantoInl('z', (3*siirtokaista) % 4);
	  break;
	default:
	  switch(tapaht.key.keysym.sym) {
	  case SDLK_RSHIFT:
	  case SDLK_LSHIFT:
	    siirtokaista++;
	    break;
	  case SDLK_RCTRL:
	  case SDLK_LCTRL:
	    kontrol = 1;
	    break;
	  case SDLK_PAUSE:
	    seis(); //tarvitaan virheenjäljitykseen (gdb: break seis)
	    break;
	  case SDLK_RETURN:
	    kaantoanimaatio(_f, 1, (koordf){{0,1,0}}, 1.0, 1);
	    break;
#define A kuva.ruutuKorostus
#define B(i) kuva.ruutuKorostus.a[i]
	  case SDLK_LEFT:
	    A = hae_ruutu(B(0), B(1)-1, B(2));
	    kuva.paivita=1;
	    break;
	  case SDLK_RIGHT:
	    A = hae_ruutu(B(0), B(1)+1, B(2));
	    kuva.paivita=1;
	    break;
	  case SDLK_UP:
	    if(kuva.ruutuKorostus.a[0] < 0) {
	      kuva.ruutuKorostus = (int3){{_f, kuutio.N/2, kuutio.N/2}};
	      kuva.paivita=1;
	      break;
	    }
	    A = hae_ruutu(B(0), B(1), B(2)-1);
	    kuva.paivita=1;
	    break;
	  case SDLK_DOWN:
	    A = hae_ruutu(B(0), B(1), B(2)+1);
	    kuva.paivita=1;
	    break;
#undef A
#undef B
#ifdef __KUUTION_KOMMUNIKOINTI__
	  case SDLK_F1:
	    lue_siirrot(ipc);
	    ipc->viesti = ipcTarkastelu;
	    viimeViesti = ipcTarkastelu;
	    break;
	  case SDLK_SPACE:
	    if(viimeViesti == ipcAloita) {
	      ipc->viesti = ipcLopeta;
	      viimeViesti = ipcLopeta;
	    } else {
	      ipc->viesti = ipcAloita;
	      viimeViesti = ipcAloita;
	    }
#endif
#ifdef __PYTHON_SAVEL__
	  case SDLK_F2:; //käynnistää tai sammuttaa sävelkuuntelijan
	    if(!savelPtr) {
	      pid_t pid1, pid2;
	      if((pid1 = fork()) < 0) {
		perror("Haarukkavirhe pid1");
		break;
	      } else if(!pid1) {
		if((pid2 = fork()) < 0) {
		  perror("Haarukkavirhe pid2");
		  exit(1);
		} else if (pid2) {
		  _exit(0);
		} else {
		  if(system("./sävel.py") < 0)
		    perror("sävel.py");
		  exit(0);
		}
	      } else
		waitpid(pid1, NULL, 0);
	      savelPtr = savelmuistiin();
	      *savelPtr = -1.0;
	      break;
	    } else {
	      savelPtr = sulje_savelmuisti((void*)savelPtr);
	    }
#endif
	  default:
	    if('1' < tapaht.key.keysym.sym && tapaht.key.keysym.sym <= '9')
	      siirtokaista += tapaht.key.keysym.sym - '1';
	    else if(SDLK_KP_1 < tapaht.key.keysym.sym && tapaht.key.keysym.sym <= SDLK_KP_9)
	      siirtokaista += tapaht.key.keysym.sym - SDLK_KP_1;
	    break;
	  }
	  break;
	}
	break;
      case SDL_KEYUP:
	switch(tapaht.key.keysym.sym) {
	case SDLK_RSHIFT:
	case SDLK_LSHIFT:
	  siirtokaista = 1;
	  break;
	case SDLK_RCTRL:
	case SDLK_LCTRL:
	  kontrol = 0;
	  break;
	default:
	  if('1' <= tapaht.key.keysym.sym && tapaht.key.keysym.sym <= '9')
	    siirtokaista = 1;
	  else if(SDLK_KP_1 <= tapaht.key.keysym.sym && tapaht.key.keysym.sym <= SDLK_KP_9)
	    siirtokaista = 1;
	  break;
	}
	break;
      case SDL_MOUSEBUTTONDOWN:
	xVanha = tapaht.button.x;
	yVanha = tapaht.button.y;
	hiiri_painettu = 1;
	if(tapaht.button.button == SDL_BUTTON_RIGHT)
	  hiiri_painettu = 2;
	break;
      case SDL_MOUSEMOTION:;
	/*pyöritetään, raahauksesta hiirellä*/
	if(hiiri_painettu) {
	  float xEro = tapaht.motion.x - xVanha; //vasemmalle negatiivinen
	  float yEro = tapaht.motion.y - yVanha; //alas positiivinen
	  xVanha = tapaht.motion.x;
	  yVanha = tapaht.motion.y;
	  kuutio.xyz.a[1] += xEro*PI/(2*kuva.resKuut);
	  if(hiiri_painettu == 2)
	    kuutio.xyz.a[2] += yEro*PI/(2*kuva.resKuut);
	  else
	    kuutio.xyz.a[0] += yEro*PI/(2*kuva.resKuut);
	  if(kuutio.xyz.a[1] < -PI)
	    kuutio.xyz.a[1] += 2*PI;
	  else if (kuutio.xyz.a[1] > PI)
	    kuutio.xyz.a[1] -= 2*PI;
	  if(kuutio.xyz.a[0] < -PI)
	    kuutio.xyz.a[0] += 2*PI;
	  else if (kuutio.xyz.a[0] > PI)
	    kuutio.xyz.a[0] -= 2*PI;
	  
	  tee_ruutujen_koordtit();
	  kuva.paivita = 1;
	}
	break;
      case SDL_MOUSEBUTTONUP:
	hiiri_painettu = 0;
	break;
      }
    } //while poll_event
#ifdef __PYTHON_SAVEL__
    if(savelPtr) {
      static char suunta = 1;
      static double savLoppuHetki = 1.0;
      register float savel = *savelPtr;
      if(savel < 0) {
	if(savLoppuHetki < 0) { //sävel päättyi juuri
	  struct timeval hetki;
	  gettimeofday(&hetki, NULL);
	  savLoppuHetki = hetki.tv_sec + hetki.tv_usec*1.0/1000000;
	} else if(savLoppuHetki > 2) { // 1 olisi merkkinä, ettei tehdä mitään
	  struct timeval hetki;
	  gettimeofday(&hetki, NULL);
	  double hetkiNyt = hetki.tv_sec + hetki.tv_usec*1.0/1000000;
	  if(hetkiNyt - savLoppuHetki > 1.0 && kuva.korostus >= 0) {
	    siirtoInl1(kuva.korostus, suunta);
	    kuva.korostus = -1;
	    savLoppuHetki = 1;
	    *savelPtr = -1.0;
	  }
	}
	goto EI_SAVELTA;
      }
      savLoppuHetki = -1.0;
      int puoliask = savel_ero(savel);
      printf("%i\n", puoliask);
      *savelPtr = -1.0;
      suunta = 1;
      if(puoliask < 0) {
	puoliask += 12;
	suunta = 3;
      }
      switch(puoliask) {
      case 0:
	kuva.korostus = _r;
	break;
      case 2:
	kuva.korostus = _l;
	break;
      case 4:
	kuva.korostus = _u;
	break;
      case 5:
	kuva.korostus = _d;
	break;
      case 7:
	kuva.korostus = _f;
	break;
      case 10:
	kuva.korostus = _b;
	break;
      default:
	goto EI_SAVELTA;
      }
      kuva.paivita = 1;
    }
  EI_SAVELTA:
#endif
    if(kuva.paivita)
      paivita();
    SDL_Delay(20);
  }
  
 ULOS:
#ifdef __PYTHON_SAVEL__
  if(savelPtr) {
    if(system("pkill sävel.py") < 0)
      printf("Sävel-ohjelmaa ei suljettu\n");
    savelPtr = NULL;
  }
#endif
  SDL_DestroyRenderer(kuva.rend);
  SDL_DestroyWindow(kuva.ikkuna);
  tuhoa_kuutio(kuutio);
  if(!oli_sdl)
    SDL_Quit();
  return 0;
}

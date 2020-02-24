#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "GrilleSDL.h"
#include "Ressources.h"
#include "Ecran.h"

// Dimensions de la grille de jeu
#define NB_LIGNE 21
#define NB_COLONNE 17

// Macros utilisees dans le tableau tab
#define VIDE         0
#define MUR          1
#define PACMAN       -2
#define PACGOM       -3
#define SUPERPACGOM  -4
#define BONUS        -5

// Autres macros
#define LENTREE 15
#define CENTREE 8

typedef struct
{
	int L;
	int C;
	int couleur;
	int cache;
} S_FANTOME;

int dir = GAUCHE;

pthread_mutex_t mutexTab;
pthread_mutex_t mutexDir;

int tab[NB_LIGNE][NB_COLONNE]
={ {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
   {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
   {1,0,1,1,0,1,1,0,1,0,1,1,0,1,1,0,1},
   {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
   {1,0,1,1,0,1,0,1,1,1,0,1,0,1,1,0,1},
   {1,0,0,0,0,1,0,0,1,0,0,1,0,0,0,0,1},
   {1,1,1,1,0,1,1,0,1,0,1,1,0,1,1,1,1},
   {1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,1,1},
   {1,1,1,1,0,1,0,1,0,1,0,1,0,1,1,1,1},
   {0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0},
   {1,1,1,1,0,1,0,1,1,1,0,1,0,1,1,1,1},
   {1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,1,1},
   {1,1,1,1,0,1,0,1,1,1,0,1,0,1,1,1,1},
   {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
   {1,0,1,1,0,1,1,0,1,0,1,1,0,1,1,0,1},
   {1,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,1},
   {1,1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,1},
   {1,0,0,0,0,1,0,0,1,0,0,1,0,0,0,0,1},
   {1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,0,1},
   {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
   {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}};

void DessineGrilleBase();
void Attente(int milli);
void VerificationCase(int, int);

void* ThreadEvent(void*);
void* ThreadPacMan(void*);
void* ThreadPacCom(void*);
void* ThreadScore(void*);
void* ThreadBonus(void*);
void* threadCompteurFantomes(void*);
void* threadFantome(void*);

void Haut(int);
void Bas(int);
void Gauche(int);
void Droite(int);

int nbPacGom = 0;
pthread_cond_t condNombre;
pthread_mutex_t mutexCompteur;

int score = 0;
bool MAJScore = false;

pthread_mutex_t mutexScore;
pthread_cond_t condScore;

int vitesse = 300;
pthread_mutex_t mutexVitesse;

int niveau = 1;

pthread_mutex_t mutexNbFantomes;
pthread_cond_t condNbFantomes;

int nbRouge =0;
int nbVert = 0;
int nbMauve = 0;
int nbOrange = 0;

pthread_key_t cle;

///////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char* argv[])
{
	struct sigaction A;

	pthread_t EventTid;
  	pthread_t PacTid;
  	pthread_t PacGomManagerTid;
 	pthread_t ScoreTid;
 	pthread_t BonusTid;
 	pthread_t CompteurFantomeTid;
 	
  	srand((unsigned)time(NULL));
  
  /* ------------ MUTEXS ----------- */  
  	pthread_mutex_init(&mutexTab,NULL);
  	pthread_mutex_init(&mutexDir, NULL);
  	pthread_mutex_init(&mutexCompteur, NULL);
  	pthread_mutex_init(&mutexScore, NULL);
  	pthread_mutex_init(&mutexVitesse, NULL);
  	pthread_mutex_init(&mutexNbFantomes, NULL);
  	
  	pthread_cond_init(&condNombre, NULL);
  	pthread_cond_init(&condScore, NULL);
  	pthread_cond_init(&condNbFantomes, NULL);
  	
  /* ------------ CREATION CLE -------------*/
  
  pthread_key_create(&cle,NULL);
  
  /* -------------- SIGNAUX --------------*/
  
  
  	A.sa_handler = Haut;
	sigemptyset(&(A.sa_mask));
	sigaddset(&(A.sa_mask), SIGUSR1);
	A.sa_flags = 0;
	sigaction(SIGUSR1, &A, NULL);
	
	A.sa_handler = Bas;
	sigemptyset(&(A.sa_mask));
	sigaddset(&(A.sa_mask), SIGUSR2);
	A.sa_flags = 0;
	sigaction(SIGUSR2, &A, NULL);
	
	A.sa_handler = Droite;
	sigemptyset(&(A.sa_mask));
	sigaddset(&(A.sa_mask), SIGHUP);
	A.sa_flags = 0;
	sigaction(SIGHUP, &A, NULL);
	
	A.sa_handler = Gauche;
	sigemptyset(&(A.sa_mask));
	sigaddset(&(A.sa_mask), SIGINT);
	A.sa_flags = 0;
	sigaction(SIGINT, &A, NULL);
	
	sigset_t mask;
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK,&mask, NULL);
  
  /* ----------------- ------------------*/
  

  // Ouverture de la fenetre graphique
	printf("(MAIN %d) Ouverture de la fenetre graphique\n",pthread_self()); fflush(stdout);
	if (OuvertureFenetreGraphique() < 0)
	{
		printf("Erreur de OuvrirGrilleSDL\n");
		fflush(stdout);
		exit(1);
	 }

	DessineGrilleBase();

	pthread_create(&PacGomManagerTid, NULL, ThreadPacCom, NULL);
	pthread_create(&ScoreTid, NULL, ThreadScore, NULL);
	pthread_create(&PacTid,NULL,ThreadPacMan,NULL);
	pthread_create(&EventTid, NULL, ThreadEvent, NULL);
	pthread_create(&BonusTid, NULL, ThreadBonus, NULL);
	pthread_create(&CompteurFantomeTid, NULL, threadCompteurFantomes, NULL);
	
	
	pthread_join(PacTid,NULL);
  
	printf("Attente de 1500 millisecondes...\n");
	Attente(1500);
	  // -------------------------------------------------------------------------
	  
	  // Fermeture de la fenetre
	printf("(MAIN %d) Fermeture de la fenetre graphique...",pthread_self()); fflush(stdout);
	FermetureFenetreGraphique();
	printf("OK\n"); fflush(stdout);

	exit(0);
}

//*********************************************************************************************
void Attente(int milli)
{
  struct timespec del;
  del.tv_sec = milli/1000;
  del.tv_nsec = (milli%1000)*1000000;
  nanosleep(&del,NULL);
}

//*********************************************************************************************
void DessineGrilleBase()
{
  for (int l=0 ; l<NB_LIGNE ; l++)
    for (int c=0 ; c<NB_COLONNE ; c++)
    {
      if (tab[l][c] == VIDE) EffaceCarre(l,c);
      if (tab[l][c] == MUR) DessineMur(l,c);
    }
}

/* ----------------------- THREADS -----------------------*/
void* ThreadPacMan(void* pt)
{
	printf("Debut du ThreadPacMan %d\n",pthread_self());
	/* ---------- LIBERATION DES SIGNAUX à RECEVOIR --------*/
	sigset_t mask;
	sigset_t oldmask;
	sigfillset(&mask);
	sigdelset(&mask, SIGINT);
	sigdelset(&mask, SIGHUP);
	sigdelset(&mask, SIGUSR1);
	sigdelset(&mask, SIGUSR2);
	sigprocmask(SIG_SETMASK,&mask, &oldmask);
	/* ---------------------------------------------------*/

	char ok;
	int L = LENTREE;
	int C = CENTREE;
	DessinePacMan(L,C,dir);  
	
	pthread_mutex_lock(&mutexTab);
	sigprocmask(SIG_SETMASK, &oldmask, NULL);
	tab[L][C] = PACMAN;	
	sigprocmask(SIG_SETMASK, &mask, NULL);
	pthread_mutex_unlock(&mutexTab);
	
	ok = 0;
  	while(!ok)
  	{
  		pthread_mutex_lock(&mutexTab);
  		sigprocmask(SIG_SETMASK, &oldmask, NULL);
  		EffaceCarre(L,C);
  		tab[L][C] = VIDE;
	  	switch(dir)
	  	{
	  		case DROITE:	if(tab[L][C+1] != MUR )	C++;
	  				break;
	  		
	  		case GAUCHE:	if(tab[L][C-1] != MUR) C--;
	  				break;
	  			
	  		case HAUT:		if(tab[L-1][C] != MUR) L--;	
					break;
		  		
	  		case BAS:		if(tab[L+1][C] != MUR) L++;
					break;  				
		}
  		
  		 VerificationCase(L,C);
			
		 DessinePacMan(L,C,dir);
		 tab[L][C] = PACMAN;
		 
		 sigprocmask(SIG_SETMASK, &mask, NULL);
		 pthread_mutex_unlock(&mutexTab);
		 
		 sigprocmask(SIG_SETMASK, &oldmask, NULL);
		 pthread_mutex_lock(&mutexVitesse);
		 Attente(vitesse);
		 pthread_mutex_unlock(&mutexVitesse);
		 sigprocmask(SIG_SETMASK, &mask, NULL);
	  }
}

void VerificationCase(int l, int c)
{
	//	pthread_mutex_lock(&mutexTab) deja fait dans le thread appelant
	if(tab[l][c] == PACGOM || tab[l][c] == SUPERPACGOM || tab[l][c] == BONUS)
	{
		if(tab[l][c] != BONUS)
		{
			pthread_mutex_lock(&mutexCompteur);
			nbPacGom --;
			pthread_mutex_unlock(&mutexCompteur);
		}
		
		pthread_mutex_lock(&mutexScore);
		if(tab[l][c] == PACGOM)
		{
			Trace("PACGOM");
			score += 1;	
		}
		if(tab[l][c] == SUPERPACGOM)
		{
			Trace("SUPERPACGOM");
			score += 5;
		}	
		if(tab[l][c] == BONUS)
		{
			Trace("BONUS");
			score += 30;
		}
				
		pthread_mutex_unlock(&mutexScore);
		
		pthread_cond_signal(&condNombre);
		pthread_cond_signal(&condScore);
	}
}

void* ThreadEvent(void*)
{
	char ok;
	EVENT_GRILLE_SDL event;
	printf("Debut du thread event: %d\n",pthread_self());
	ok = 0;
  	while(!ok)
  	{
		event = ReadEvent();
		if (event.type == CROIX) ok = 1;
		if (event.type == CLAVIER)
		{
		  switch(event.touche)
		  {
		    case 'q' : ok = 1; break;
		    case KEY_RIGHT : /*printf("Fleche droite !\n");*/ 	kill(getpid(), SIGHUP); 	break; // signaux à envoyer
		    case KEY_LEFT : /*printf("Fleche gauche !\n");*/ 	kill(getpid(), SIGINT); 	break;
		    case KEY_DOWN : /*printf("Fleche bas !\n");*/ 		kill(getpid(), SIGUSR2); 	break;
		    case KEY_UP : /*printf("Fleche haut !\n");*/ 		kill(getpid(), SIGUSR1); 	break;
		  	// ajouter touche de triche -- easteregg
		  }
		}
	}
	exit(0);
}

void* ThreadPacCom(void* pt)
{
	printf("Debut du ThreadPacCom %d\n",pthread_self());
	int i, j;
	
	while(1)
	{
		DessineChiffre(14,22,niveau);
		
		pthread_mutex_lock(&mutexTab);
	
	// dessine pacGom à toutes les cases vides -- EXCEPT (15.8) (8.8) (9.8)
	for(i = 0; i < NB_LIGNE ; i++)
		for(j = 0; j < NB_COLONNE; j++)
		{
				if ((i == 15 && j == 8) || (i == 8 && j == 8) || (i == 9 && j == 8))
					j++;
				
				if(tab[i][j] == VIDE)
				{
					nbPacGom++;
					DessinePacGom(i,j);
					tab[i][j] = PACGOM;
				}
				
		}
	
		// dessine SuperPacGom (2.1) (2.15) (15.1) (15.15)	
		DessineSuperPacGom(2,1);
		tab[2][1] = SUPERPACGOM;	
		// nbPacGom++; on remplace un PacGom donc pas besoin
	
		DessineSuperPacGom(2,15);
		tab[2][15] = SUPERPACGOM;
	
		DessineSuperPacGom(15,1);
		tab[15][1] = SUPERPACGOM;
	
		DessineSuperPacGom(15,15);
		tab[15][15] = SUPERPACGOM;
	
		pthread_mutex_unlock(&mutexTab);
	
	
	
		pthread_mutex_lock(&mutexCompteur);
		DessineChiffre(12,22,nbPacGom/100); DessineChiffre(12,23,(nbPacGom%100)/10); DessineChiffre(12,24,nbPacGom%10); // init
	
		while(nbPacGom > 0)
		{
			pthread_cond_wait(&condNombre,&mutexCompteur);
			DessineChiffre(12,22,nbPacGom/100); DessineChiffre(12,23,(nbPacGom%100)/10); DessineChiffre(12,24,nbPacGom%10); // refresh de l'ecran
		}
	
		pthread_mutex_unlock(&mutexCompteur);
	
		#ifdef DEV
		Trace("\nTU AS TOUT MANGE \n");
		#endif
		
		
		// TACHES A FAIRE -- INCREMENTER LE NIVEAU DE 1
		niveau++;
		
		// DIVISER LA VITESSE DU PACMAN PAR 2 
		pthread_mutex_lock(&mutexVitesse);
		vitesse = vitesse/2;
		pthread_mutex_unlock(&mutexVitesse);
		// REINITIALISER LE TABLEAU
	
	}
}

void* ThreadScore(void* pt)
{
	printf("Debut du ThreadScore %d\n",pthread_self());
	pthread_mutex_lock(&mutexScore);
	
	while(MAJScore == false)
	{
		pthread_cond_wait(&condScore,&mutexScore);
		DessineChiffre(16,22,score/1000);
		DessineChiffre(16,23,(score%1000)/100);
		DessineChiffre(16,24,(score%100)/10);
		DessineChiffre(16,25,score%10);
		MAJScore = false;
	}
	
	pthread_mutex_unlock(&mutexScore);
}

void* ThreadBonus(void* pt)
{
	printf("Debut du ThreadBonus %d\n",pthread_self());
	int laps;
	int l,c;
	bool trouve = false;
	
	while(1)
	{	
		laps = rand()%10000+10000;
		printf("Prochain bonus dans: %d ms\n",laps);
		Attente(laps);
		while(!trouve)
		{
			l = rand()%NB_LIGNE;
			c = rand()%NB_COLONNE;
			pthread_mutex_lock(&mutexTab);
			
			if(tab[l][c] == VIDE)
				trouve = true;
			
			pthread_mutex_unlock(&mutexTab);
		}
		trouve = false;
		Trace("PLACE VIDE %d - %d \n",l,c);
		
		pthread_mutex_lock(&mutexTab);
		DessineBonus(l,c);
		tab[l][c] = BONUS;
		pthread_mutex_unlock(&mutexTab);
		
		// depose un objet BONUS
		Attente(10000);
		pthread_mutex_lock(&mutexTab);
		if(tab[l][c] == BONUS)
		{
			tab[l][c] == VIDE;
			EffaceCarre(l,c);
		}	
		
		pthread_mutex_unlock(&mutexTab);
	}
}

void* threadCompteurFantomes(void* pt)
{
	printf("Debut du ThreadCompteurFantomes %d\n",pthread_self());
// ou initialiser les fantomes ? + alloc dynamique;

	pthread_mutex_lock(&mutexNbFantomes);
	while(nbRouge == 2 && nbVert == 2 && nbMauve == 2 && nbOrange == 2)
	{
		pthread_cond_wait(&condNbFantomes,&mutexNbFantomes);
	}
	pthread_mutex_unlock(&mutexNbFantomes);
}

void* threadFantome(void* pt)
{
	printf("Debut d'un thread Fantome %d\n",pthread_self());
	// zone spécifique = (S_FANTOME*)pt;
	pthread_setspecific(cle,pt);
}
/* ------------ SIGNAUX ------------ */
void Haut(int sig)
{
	#ifdef DEV
	printf("\tSignal Haut %d par Thread%d\n",sig,pthread_self());
	#endif
	pthread_mutex_lock(&mutexDir);
	dir = HAUT;
	pthread_mutex_unlock(&mutexDir);
}

void Bas(int sig)
{
	#ifdef DEV
	printf("\tSignal Bas %d par Thread%d\n",sig,pthread_self());
	#endif
	pthread_mutex_lock(&mutexDir);
	dir = BAS;
	pthread_mutex_unlock(&mutexDir);
}

void Gauche(int sig)
{
	#ifdef DEV
	printf("\tSignal Gauche %d par Thread%d\n",sig,pthread_self());
	#endif
	pthread_mutex_lock(&mutexDir);
	dir = GAUCHE;
	pthread_mutex_unlock(&mutexDir);
	
	struct sigaction A;
	A.sa_handler = Gauche;
	sigemptyset(&(A.sa_mask));
	sigaddset(&(A.sa_mask), SIGINT);
	A.sa_flags = 0;
	sigaction(SIGINT, &A, NULL);
	
}

void Droite(int sig)
{
	#ifdef DEV
	printf("\tSignal Droite %d par Thread%d\n",sig,pthread_self());
	#endif
	pthread_mutex_lock(&mutexDir);
	dir = DROITE;
	pthread_mutex_unlock(&mutexDir);
}


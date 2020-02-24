#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "GrilleSDL.h"
#include "Ressources.h"

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

void* ThreadEvent(void*);
void* ThreadPacMan(void*);

void Haut(int);
void Bas(int);
void Gauche(int);
void Droite(int);

///////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char* argv[])
{
	struct sigaction A;
	
	pthread_t EventTid;
  	pthread_t PacTid;
 
  	srand((unsigned)time(NULL));
  
  /* ------------ MUTEXS ----------- */  
  	pthread_mutex_init(&mutexTab,NULL);
  	pthread_mutex_init(&mutexDir, NULL);
  	
  /* -------------- SIGNAUX --------------*/
  
  	A.sa_handler = Haut;
	sigemptyset(&A.sa_mask);
	sigaddset(&A.sa_mask, SIGUSR1);
	A.sa_flags = 0;
	sigaction(SIGUSR1, &A, NULL);
	
	A.sa_handler = Bas;
	sigemptyset(&A.sa_mask);
	sigaddset(&A.sa_mask, SIGUSR2);
	A.sa_flags = 0;
	sigaction(SIGUSR2, &A, NULL);
	
	A.sa_handler = Droite;
	sigemptyset(&A.sa_mask);
	sigaddset(&A.sa_mask, SIGHUP);
	A.sa_flags = 0;
	sigaction(SIGHUP, &A, NULL);
	
	A.sa_handler = Gauche;
	sigemptyset(&A.sa_mask);
	sigaddset(&A.sa_mask, SIGINT);
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

	  // Exemple d'utilisation de GrilleSDL et Ressources --> code a supprimer
	DessineChiffre(14,25,9);
	//  DessineFantome(5,9,ROUGE,DROITE);
	//  DessinePacGom(7,4);
	//  DessineSuperPacGom(9,5);
	//  DessineFantomeComestible(13,15);
	//  DessineBonus(5,15);
	  
	pthread_create(&PacTid,NULL,ThreadPacMan,NULL);
	pthread_create(&EventTid, NULL, ThreadEvent, NULL);
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
	  		case DROITE: 
	  			{
	  				if(tab[L][C+1] == VIDE)
	  					C++;
	  				break;
	  			}
	  		
	  		case GAUCHE: 
	  			{
	  				if(tab[L][C-1] == VIDE)
	  					C--;
	  				break;
	  			}
	  			
	  		case HAUT:
		  		{
		  			if(tab[L-1][C] == VIDE)
	  					L--;	
					break;
		  		}
		  		
	  		case BAS:
	  			{
	  				if(tab[L+1][C] == VIDE)
						L++;
					break;  				
	  			}
  		}
  
			
		 DessinePacMan(L,C,dir);
		 tab[L][C] = PACMAN;
		 sigprocmask(SIG_SETMASK, &mask, NULL);
		 pthread_mutex_unlock(&mutexTab);
		 
		 Attente(300);
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
		    case KEY_RIGHT : printf("Fleche droite !\n"); kill(getpid(), SIGHUP); break; // signaux à envoyer
		    case KEY_LEFT : printf("Fleche gauche !\n"); kill(getpid(), SIGINT); break;
		    case KEY_DOWN : printf("Fleche bas !\n"); kill(getpid(), SIGUSR2); break;
		    case KEY_UP : printf("Fleche haut !\n"); kill(getpid(), SIGUSR1); break;
		    default: printf("Une autre touche .. \n");
		  }
		}
	}
}


/* ------------ SIGNAUX ------------ */
void Haut(int sig)
{
	printf("\tSignal Haut %d\n",sig);
	pthread_mutex_lock(&mutexDir);
	dir = HAUT;
	pthread_mutex_unlock(&mutexDir);
}

void Bas(int sig)
{
	printf("\tSignal Bas %d\n",sig);
	pthread_mutex_lock(&mutexDir);
	dir = BAS;
	pthread_mutex_unlock(&mutexDir);
}

void Gauche(int sig)
{
	printf("\tSignal Gauche %d\n",sig);
	pthread_mutex_lock(&mutexDir);
	dir = GAUCHE;
	pthread_mutex_unlock(&mutexDir);
}

void Droite(int sig)
{
	printf("\tSignal Droite %d\n",sig);
	pthread_mutex_lock(&mutexDir);
	dir = DROITE;
	pthread_mutex_unlock(&mutexDir);
}


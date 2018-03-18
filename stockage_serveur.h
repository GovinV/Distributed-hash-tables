#ifndef __STOCKAGE_SERVEUR_H__
#define __STOCKAGE_SERVEUR_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>

typedef unsigned short taille;
typedef unsigned char donnees;

typedef struct emplacement{
    donnees *adresse;           // Adresse IP associee a un hash
    taille taille_adresse;      // Taille de la chaine adresse
    long int obsolescence;      // Timer de la derniere mise a jour de la donnee
    struct emplacement *next;   // Pointeur sur la prochaine adresse IP associee
} l_emplacement;

typedef struct stockage{
    donnees *hash;              // Chaine representant un hash
    taille taille_hash;         // Taille de la chaine hash
    struct emplacement *dispo;  // Pointeur sur la liste des adresses IP
                                // associees au hash
    struct stockage *next;      // Pointeur sur le hash suivant
} l_hash;

typedef struct a_serveurs
{
    struct sockaddr * serveur;  // Structure contenant les informations
                                // necessaire pour contacter ce serveur
    socklen_t addrlen;          // Longueur de la structure serveur
    int present;                // Indique si le serveur est suppose present
    struct a_serveurs *next;    // Pointeur sur le a_serveur suivant
} l_serveur;


/* Libere recursivement la memoire attribuee a une liste d'emplacement */
void delete_l_emplacement(l_emplacement *emp);

/* Creer une nouvelle structure l_emplacement et l'initialise */
int new_emplacement(l_emplacement **retour, donnees* adresse, 
                        taille taille_adresse);

/* Ajoute un emplacement (une adresse IP) a une liste d'emplacement */
int add_emplacement(l_emplacement **retour, donnees* adresse, 
                        taille taille_adresse);

/* Libere recursivement la memoire attribuee la liste de hash */
void delete_l_hash(l_hash* table);

/* Creer une nouvelle structure l_hash et l'initialise */
int new_hash(l_hash **retour, donnees* hash, taille taille_hash, 
                donnees* adresse, taille taille_adresse);

/* Ajoute un hash a la liste des hash repertories par le serveur */
int add_hash(l_hash **retour, donnees* hash, taille taille_hash, 
                donnees* adresse, taille taille_adresse);

/* Libere recursivement la memoire attribuee la liste de serveurs */
void delete_l_serveurs(l_serveur *serveurs);

/* Creer une nouvelle structure l_serveur et l'initialise */
int new_a_serveurs(l_serveur **retour,
                    struct sockaddr* serveur, socklen_t addrlen);

/* Ajoute un serveur a une liste de serveurs */
int add_a_serveurs(l_serveur **debut, 
                    struct sockaddr* serveur, socklen_t addrlen);

/* Supprime un serveur à partir de son adresse Ip et son port */
void delete_server(l_serveur **debut, struct sockaddr* serveur);

/* Compare les ports de deux structures sockaddr */
int sockaddr_cmp(struct sockaddr *x, struct sockaddr *y);

/* Recupere l'adresse d'une struct sockaddr dans une chaine de caractere */
void get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen);

/* Compare deux structure sockaddr contenant les informations de serveurs */
int sockaddrcmp(struct sockaddr *x, struct sockaddr *y);

#endif

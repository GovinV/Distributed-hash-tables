#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>  
#include <sys/time.h>
#include <time.h>

#define FALSE 0
#define TRUE !FALSE

#define SERVEUR 1
#define CLIENT 2

/* Codes de retour particuliers */
#define CODE_CANCEL_WAIT 98
#define CODE_INTERRUP_SYSTEM 99

/* Temps d'attente avant que le client arrete d'attendre une reponse */
#define CLIENT_TIMEOUT_SEC 2
#define CLIENT_TIMEOUT_MICROSEC 0

/* Donnees pour la gestion des check-alive */
#define SERVEUR_CHK_A_SEC 5
#define SERVEUR_CHK_A_MICROSEC 0

/* Duree avant qu'une donnee soit obsolete */
#define TEMPS_OBSOLESCENCE 30

/* Donnees relatives a l'entete d'un message */
#define SIZEOF_TYPE 1
#define SIZEOF_TAILLE 2
#define SIZEOF_ENTETE SIZEOF_TYPE+SIZEOF_TAILLE

/* Donnees relatives a l'entete d'un bloc de donnees dans un message */
#define SIZEOF_TYPE_BLOC 1
#define SIZEOF_TAILLE_BLOC 2
#define SIZEOF_ENTETE_BLOC SIZEOF_TYPE_BLOC+SIZEOF_TAILLE_BLOC

/* 2^sizeof(taille)-1 */
#define MAX_MESS_SIZE 65535

typedef unsigned short taille;
typedef unsigned char donnees;

typedef struct sockaddr_in6 sockaddr_in;
typedef struct sockaddr sockaddr;

typedef struct{
            unsigned int lg_allouee;    // Nombre d'octets alloues a contenu
            unsigned int lg_message;    // Nombre d'octets utilises
            donnees *contenu;           // Suite d'octets a envoyer
            donnees type;               // Type du message :
                                        // - g pour get
                                        // - p pour put
                                        // - r pour reponse
                                        // - n pour new server
                                        // - f pour fin transfert
                                        // - k pour keep-alive
                                        // - a pour alive
                                        // - d pour deconnexion
                                        // - t pour transfert
} message;
/* 
 Type des bloc de donnees dans le message : 
 - a pour adresse
 - h pour hash
 - s pour serveur
*/

/* Creer un nouveau message */
int create_message(message **m, donnees type, unsigned int lg);

/* Creer un message apres lecture d'une entete */
int create_message_with_header(message **m, donnees *header);

/* Libere l'espace alloue par un message */
void delete_message(message *m);

/* Ajoute un bloc dans un message */
int add_data(message *m, donnees type, taille lg, void* data);

/* Ecrit la taille totale du message dans l'entete du message */
void prepare_message(message *m);

/* Receptionne un message */
int recevoir_message(message **m, int sfd, struct sockaddr *client, socklen_t *addrlen);

/* Lit un message et renvoie le hash suivant trouve */
int message_get_h(message *m, donnees** emplacement, taille *taille_lue);

/* Lit un message et renvoie l'adresse suivante trouvee */
int message_get_a(message *m, donnees** emplacement, taille *taille_lue);

/* Lit un message et renvoie le serveur suivant trouve */
int message_get_s(message *m, struct sockaddr **serveur, taille *taille_lue);

/* Recherche parmis toute les adresses possibles une adresse valide */
int get_addr(int role, char *adresse, char* port, int *sockfd, 
                    struct addrinfo **debut, struct addrinfo **valide);

#endif

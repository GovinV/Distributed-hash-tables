#include "messages.h"

/**
 * @brief Affiche l'usage correct du programme.
 *
 * @param nom_prgm le nom du programme recupere via la ligne de commande.
*/
void print_usage(char *nom_prgm)
{
    fprintf(stderr, "Usages : %s IP PORT GET HASH \n"\
                    "         %s IP PORT PUT HASH IP\n", nom_prgm, nom_prgm);
    exit(1);
}

/**
 * @brief Affiche toutes les adresses IP contenues dans un message.
 *
 * @param m un pointeur sur le message a lire.
 * @return 0 en cas de reussite, 3 ou 4 si write rencontre un probleme.
*/
int afficher_adresse_dispo(message *m)
{
    donnees *adresse;
    taille taille_adresse;

    /* Passe une premiere fois le message en parametre et recupere
       la premiere adresse (l'absence d'adresse n'est pas consideree
       comme une erreur) */
    if(message_get_a(m, &adresse, &taille_adresse)==-1)
        return 0;
    
    /* Puis ecrit les adresses recuperees tant qu'il y en a */
    do
    {
        if(write(1, adresse, taille_adresse)==-1)
        {
            perror("Error write");
            return 3;
        }
        
        if(write(1, " ", 1)==-1)
        {
            perror("Error write");
            return 4;
        }
    }
    while(message_get_a(NULL, &adresse, &taille_adresse)!=-1);
    
    return 0;
}

/**
 * @brief Simule un client communiquant avec un serveur.
 *
 * Le client peux au choix stocker sur un serveur un hash et l'adresse a
 * laquelle les donnees associees au hash peuvent etre telechargees, ou alors
 * demander a un serveur a quelles adresses il est possible de telecharger les
 * donnees associees a un hash.
 *
 * @param argv[1] IP l'ip du serveur a contacter.
 * @param argv[2] PORT port du serveur avec lequel discuter.
 * @param argv[3] une commande, soit GET, soit PUT.
 * @param argv[4] HASH le hash a demander ou a stocker (selon la commande).
 * @param argv[5] IP l'ip correspondant a la machine contenant les donnees
 *                associees au hash (argv[4]) dans le cas d'une commande PUT.
*/
int main(int argc, char * argv[])
{
    int sockfd, err;
    message *m, *m2;
	struct addrinfo *head, *valide;
    struct timeval timeout = {CLIENT_TIMEOUT_SEC,CLIENT_TIMEOUT_MICROSEC};

    if(argc == 5) /* Cas d'une commande get */
    {
        /* Teste si la commande n'est ni "get", ni "GET" */
        if((strcmp(argv[3],"get") != 0 && strcmp(argv[3],"GET") != 0))
        {
            print_usage(argv[0]);
        }
        
        /* Cree un nouveau message de type 'g' */
        err=create_message(&m, 'g', SIZEOF_ENTETE);
        if(err!=0)
        {
            exit(err);
        }
        
        /* Ajoute dans le message le hash a demander */
        err=add_data(m, 'h', strlen(argv[4])+1, argv[4]);
        if(err!=0)
        {
            delete_message(m);
            exit(err);
        }
        
        /* Prepare le message pour l'envoie */
        prepare_message(m);
    }
    else if(argc==6) /* Cas d'une commande put */
    {
        /* Teste si la commande n'est ni "put", ni "PUT" */
        if((strcmp(argv[3],"put") != 0 && strcmp(argv[3],"PUT") != 0))
        {
            print_usage(argv[0]);
        }
        
        /* Cree un nouveau message de type 'p' */
        err=create_message(&m, 'p', SIZEOF_ENTETE);
        if(err!=0)
        {
            exit(err);
        }
        
        /* Ajoute dans le message le hash associe a l'adresse */
        err=add_data(m, 'h', strlen(argv[4])+1, argv[4]);
        if(err!=0)
        {
            delete_message(m);
            exit(err);
        }
        
        /* Ajoute dans le message l'adresse associee au hash */
        err=add_data(m, 'a', strlen(argv[5])+1, argv[5]);
        if(err!=0)
        {
            delete_message(m);
            exit(err);
        }
        
        /* Prepare le message pour l'envoie */
        prepare_message(m);
    }
    else
    {
        /* Dans le cas ou le nombre d'arguments ne correspond 
           ni a une commande get ni a une commande put */
        print_usage(argv[0]);
    }
    
    /* Recuperation d'une adresse valide pour contacter le serveur */
    err=get_addr(CLIENT, argv[1], argv[2], &sockfd, &head, &valide);
    if(err!=0)
    {
        delete_message(m);
        exit(err);
    }

    /* Envoie la requete au serveur */
    if(sendto(sockfd, m->contenu, m->lg_message, 0,
              valide->ai_addr, valide->ai_addrlen) == -1)
    {
        perror("Error sendto");
        close(sockfd);
        freeaddrinfo(head);
        delete_message(m);
        exit(2);
    }

    freeaddrinfo(head);
    
    /* Si la commande est "GET", le client attend une reponse du serveur */
    if(m->type=='g')
    {
        /* Indique que l'attente d'un message s'arrete si le temps depasse
           celui specifie dans la structure timeout */
        if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
                        &timeout, sizeof(struct timeval))==-1)
        {
            perror("Error setsockopt");
            close(sockfd);
            delete_message(m);
            exit(5);
        }
        
        /* Reception de la reponse du serveur */
        err = recevoir_message(&m2, sockfd, NULL, NULL);
        if(err!=0)
        {
            if(err==CODE_CANCEL_WAIT)
                fprintf(stderr, "Le serveur ne répond pas.\n");
            
            close(sockfd);
            delete_message(m);
            exit(err);
        }
        
        /* Affichage des adresses contenues dans la reponse du serveur */
	    printf("IP disponibles pour le téléchargement :\n");
	    err=afficher_adresse_dispo(m2);
	    if(err!=0)
	    {
	        delete_message(m2);
	        delete_message(m);
	        close(sockfd);
	        exit(err);
	    }
	    printf("\n");
	    
	    delete_message(m2);
    }

    delete_message(m);
    close(sockfd);

    return 0;
}

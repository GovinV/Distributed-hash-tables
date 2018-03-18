#include "messages.h"
#include "stockage_serveur.h"

// Permet d'arreter le serveur proprement.
int serveur_actif = TRUE;

// Permet de verifier si les reponses des keep-alive ont ete reçues.
int check_K_A = FALSE;

/**
 * @brief Fonction appelee lorsque le programme reçoit le signal SIGINT.
 *
 * Informe le serveur qu'il doit s'arreter.
 *
 * @param val la valeur du signal reçu (ignoree).
*/
void arret_serveur(__attribute__((unused)) int val)
{
    serveur_actif = FALSE;
}

/**
 * @brief Fonction appelee lorsque le programme reçoit le signal SIGALRM.
 *
 * Informe le serveur qu'il doit vérifier que les reponses aux
 * keep-alive ont ete reçues.
 *
 * @param val la valeur du signal reçu (ignoree).
*/
void check_keep_alive(__attribute__((unused)) int val)
{
    check_K_A = TRUE;
}

/**
 * @brief Gere les actions a effectuer en fonction des signaux reçus.
 *
 * - Change l'action par default lors de la reception du signal SIGINT
 *   (appel de la fonction arret_serveur).
 * - Change l'action par default lors de la reception du signal SIGALRM
 *   (appel de la fonction check_keep_alive).
 *
 * @return 0 en cas de reussite, un code d'erreur sinon.
*/
int gestion_signaux()
{
    struct sigaction sig;

    /* Changement de l'action par default pour SIGINT */
    
    if(sigemptyset(&sig.sa_mask)==-1)
    {
        perror("Error sigemptyset");
        return 1;
    }
    
    /* Masquage de SIGALRM pendant l'execution de la fonction pour SIGINT */
    sigaddset(&sig.sa_mask, SIGALRM);
    sig.sa_handler = arret_serveur;
    sig.sa_flags = 0;
    
    if(sigaction(SIGINT, &sig, NULL)==-1)
    {
        perror("Error sigaction");
        return 2;
    }
    
    /* Changement de l'action par default pour SIGALRM */
    
    if(sigemptyset(&sig.sa_mask)==-1)
    {
        perror("Error sigemptyset");
        return 3;
    }
    
    /* Masquage de SIGINT pendant l'execution de la fonction pour SIGALRM */
    sigaddset (&sig.sa_mask, SIGINT);
    sig.sa_handler = check_keep_alive;
    sig.sa_flags = 0;
    
    if(sigaction(SIGALRM, &sig, NULL)==-1)
    {
        perror("Error sigaction");
        return 4;
    }
    
    return 0;
}

/**
 * @brief Lis le message et ajoute au DHT un hash et son adresse associee.
 *
 * Lecture du message pour recuperer le hash et l'adresse, puis ajout d'un
 * element a la liste des hash, et envoie du hash aux autres serveurs si c'est
 * demande (si st est non NULL).
 *
 * @param m un pointeur sur le message reçu.
 * @param dht un pointeur vers le pointeur sur le debut de la liste de hash.
 * @param st un pointeur vers le pointeur sur le debut de la liste de serveur.
 * @param sockfd l'identifiant d'un socket.
 * @return 0 en cas de reussite, un code d'erreur sinon.
*/
int serveur_put(message *m, l_hash **dht, l_serveur **st, int *sockfd)
{
    int err;
    donnees *hash, *adresse;
    taille taille_hash, taille_adresse;
    l_serveur *emp;

    /* Recuperation du hash dans le message */
    if(message_get_h(m, &hash, &taille_hash)==-1)
    {
        fprintf(stderr, "Erreur : Le message ne contenait pas de hash\n");
        return 5;
    }
    
    /* Recuperation de l'adresse dans le message */
    if(message_get_a(m, &adresse, &taille_adresse)==-1)
    {
        fprintf(stderr, "Erreur : Le message ne contenait pas d'adresse\n");
        return 6;
    }
    
    /* Ajout du hash et son adresse associee dans la table de hashage */
    err=add_hash(dht, hash, taille_hash, adresse, taille_adresse);
    if(err!=0)
        return err;

    /* Si la liste de serveurs n'est pas donnee ( = NULL) alors il n'y a rien
       d'autre a faire */
    if(st==NULL || sockfd==NULL)
        return 0;
    else
        emp = *st;

    /* Changement du type du message pour qu'il soit correctement interprete */
    m->contenu[0]='t';

    /* Parcours de la liste de serveurs */
    for(;emp!=NULL;emp=emp->next)
    {
        /* Envoie le message a un serveur */
        if(sendto(*sockfd, m->contenu, m->lg_message, 0,
                  emp->serveur, emp->addrlen) == -1)
        {
            perror("Error sendto");
            return 7;
        }
    }

    return 0;
}

/**
 * @brief Recupere toute les adresses ip associees a un hash.
 *
 * Lit le message reçu, en extrait le hash en question, parcours la liste
 * des hash jusqu'a le trouver, puis ajoute a un nouveau message toutes les
 * adresses ip associees.
 *
 * @param retour un pointeur vers un pointeur sur le message qui sera la reponse
 *        du serveur (valeur de retour par effet de bord).
 * @param m un pointeur sur le message recu par le serveur.
 * @param dht un pointeur vers le premier element de la liste de hash.
 * @return 0 en cas de reussite, un code d'erreur sinon.
*/
int serveur_get(message **retour, message *m, l_hash *dht)
{
    int err;
    message *m2;
    donnees *hash;
    l_emplacement *emp;
    taille taille_hash;  
    
    /* Recupere le hash dans le message */
    if(message_get_h(m, &hash, &taille_hash)==-1)
    {
        fprintf(stderr, "Erreur : Le message ne contenait pas de hash.\n");
        return 8;
    }
    
    /* Creer un message de type reponse */
    err=create_message(&m2, 'r', SIZEOF_ENTETE);
    if(err!=0)
        return err;
    
    /* Pour chaque element de la liste de hash */
    for(; dht!=NULL; dht=dht->next)
    {
        /* Si on a trouve le hash dans la liste */
        if(taille_hash == dht->taille_hash &&
            strncmp((char*)hash, (char*)dht->hash, taille_hash)==0)
        {
            /* Pour chaque element de la liste d'adresse ip */
            for(emp=dht->dispo; emp!=NULL; emp=emp->next)
            {
                /* On ajoute l'adresse ip au message */
                err=add_data(m2, 'a', emp->taille_adresse, emp->adresse);
                if(err!=0)
                {
                    delete_message(m2);
                    return err;
                }
            }
            break;
        }
    }

    /* Passage du message par effet de bord */
    *retour = m2;
    
    return 0;
}

/**
 * @brief Envoie sa table de hash a un nouveau serveur.
 *
 * Envoie par couple (hash,adresse), toute la table de hashage a un nouveau
 * serveur se connectant au serveur courant.
 *
 * @param sockfd l'identifiant du socket a utiliser.
 * @param nouveau_serv un pointeur vers la structure contenant les informations
 *        necessaires pour parler au nouveau serveur.
 * @param addrlen la longueur de nouveau_serv.
 * @param dht un pointeur vers le debut de la table de hashage.
 * @param st un pointeur vers le debut de la liste de serveurs connus.
 * @return 0 en cas de reussite, un code d'erreur sinon.
*/
int serveur_send_all(int sockfd, struct sockaddr *nouveau_serv, 
                            socklen_t addrlen, l_hash *dht, l_serveur* st)
{
    int err;
    message *m2;
    l_emplacement * emp;

    /* Cree un nouveau message de type transfert */
    err=create_message(&m2, 't', SIZEOF_ENTETE);
    if(err!=0)
        return err;

    /* Pour chaque element de la liste de hash */
    for(; dht!=NULL; dht=dht->next)
    {        
        /* Pour chaque element de la liste d'adresse ip */
        for(emp = dht->dispo; emp!=NULL; emp=emp->next)
        {
            /* Reutilisation du meme message a chaque envoie en reecrivant
               sur les donnees precedentes */
            m2->lg_message = SIZEOF_ENTETE;            
            
            /* Ajout du hash dans le message */
            err=add_data(m2, 'h', dht->taille_hash, dht->hash);
            if(err!=0)
            {
                delete_message(m2);
                return err;
            }
            
            /* Ajout de l'adresse associee au hash dans le message */
            err=add_data(m2, 'a', emp->taille_adresse, emp->adresse);
            if(err!=0)
            {
                delete_message(m2);
                return err;
            }
            
            prepare_message(m2);
            
            /* Envoie un couple hash/adresse au nouveau serveur */
            if(sendto(sockfd, m2->contenu, m2->lg_message, 0,
                        nouveau_serv, addrlen) == -1)
            {
                perror("Error sendto");
                delete_message(m2);
                return 9;
            }
        }
    }
    
    delete_message(m2);
    
    /* Cree un nouveau message de type transfert */
    err=create_message(&m2, 't', SIZEOF_ENTETE);
    if(err!=0)
        return err;
    
    for(; st!=NULL; st=st->next)
    {
        /* Reutilisation du meme message a chaque envoie en reecrivant
           sur les donnees precedentes */
        m2->lg_message = SIZEOF_ENTETE;            
        
        /* Ajout du serveur dans le message */
        err=add_data(m2, 's', st->addrlen, st->serveur);
        if(err!=0)
        {
            delete_message(m2);
            return err;
        }
        
        prepare_message(m2);
        
        /* Envoie des donnees au serveur */
        if(sendto(sockfd, m2->contenu, m2->lg_message, 0,
                    nouveau_serv, addrlen) == -1)
        {
            perror("Error sendto");
            delete_message(m2);
            return 18;
        }
    }
    
    delete_message(m2);
    
    /* Cree un nouveau message de type fin */
    err=create_message(&m2, 'f', SIZEOF_ENTETE);
    if(err!=0)
        return err;

    prepare_message(m2);
    
    /* Envoie un message signifiant la fin du transfert du DHT */
    if(sendto(sockfd, m2->contenu, m2->lg_message, 0,
                nouveau_serv, addrlen) == -1)
    {
        perror("Error sendto");
        delete_message(m2);
        return 10;
    }
    
    delete_message(m2); 

    return 0;
}

/**
 * @brief Informe tout les serveurs connus de l'arret du serveur courant.
 *
 * Envoie un message de type 'd' (deconnexion) a tout les serveurs de la liste 
 * de serveurs pour les informer que le programme va s'arreter.
 *
 * @param st un pointeur sur le debut de la liste de serveurs.
 * @param sockfd l'identifiant du socket a utiliser.
 * @return 0 en cas de reussite, un code d'erreur sinon.
*/
int informer_arret_serveur(l_serveur *st, int sockfd)
{
    int err;
    message *m;
    l_serveur *serv;

    /* Cree un nouveau message de type deconnexion */
    err=create_message(&m, 'd', SIZEOF_ENTETE);
    if(err!=0)
        return err;
    
    prepare_message(m);

    for(serv=st; serv!=NULL; serv=serv->next)
    {
        /* Envoie le message de deconnexion a un serveur */
        if(sendto(sockfd, m->contenu, m->lg_message, 0,
                  serv->serveur, serv->addrlen) == -1)
        {
            perror("Error sendto");
            delete_message(m);
            return 11;
        }
    }

    delete_message(m);

    return 0;
}

/**
 * @brief Supprime les serveurs qui ne repondent pas et questionne les autres.
 *
 * @param st un pointeur vers le pointeur sur le debut de liste de serveurs
 *        (sa valeur est modifiee si le/les serveurs en tete de liste sont
 *         supprimes).
 * @param sockfd l'identifiant d'un socket.
 * @return 0 en cas de reussite, un code d'erreur sinon.
*/
int check_send_KA(l_serveur **st, int sockfd)
{
    int err;
    message *m;
    l_serveur *actu, *suivant, *prec;
    
    if(*st == NULL)
        return 0;
    
    actu = *st;
    suivant = NULL;
    prec = NULL;
    
    /* Creer un nouveau message de type keep-alive */
    err=create_message(&m, 'k', SIZEOF_ENTETE);
    if(err!=0)
        return err;
    
    prepare_message(m);
    
    /* Parcours de la liste de serveurs */
    while(actu!=NULL)
    {
        /* Suppression d'un serveur n'ayant pas repondu */
        if(actu->present==0)
        {
            suivant = actu->next;
            free(actu->serveur);
            free(actu);
            actu = suivant;
            printf("Serveur déconnecté: pas de réponse au keep-alive\n");
            /* Si il y as un element avant, on indique sont suivant, sinon
               on change la valeur du pointeur sur la tete de liste */
            if(prec != NULL)
            {
                prec->next = actu;
            }
            else
            {
                *st = actu;
            }
        }
        else /* Renvoie d'un message keep-alive a un serveur ayant repondu */
        {
            if(sendto(sockfd, m->contenu, m->lg_message, 0,
                      actu->serveur, actu->addrlen) == -1)
            {
                perror("Error sendto");
                delete_message(m);
                return 12;
            }
            
            actu->present=0;
            prec = actu;
            actu = actu->next;
        }
    }
    
    delete_message(m);
    
    return 0;
}

/**
 * @brief Enregistre le fait qu'un serveur soit toujours actif.
 *
 * @param st un pointeur sur le debut de la liste de serveurs.
 * @param serv un pointeur sur la structure contenant les informations du
 *        serveur ayant repondu.
*/
void isAlive(l_serveur *st, struct sockaddr *serv)
{
    l_serveur *emp;
    
    /* Parcours de la liste de serveur a la recherche de serv */
    for(emp=st; emp!=NULL; emp=emp->next)
    {
        if(sockaddrcmp(serv,emp->serveur)==0)
        {
            emp->present=1;
            break;
        }        
    }    
}

/**
 * @brief Gere l'obsolescence des adresses associees aux hashs
 *
 * Parcours l'ensemble de la table en supprimant l'ensemble des donnees
 * etant devenue obsoletes.
 *
 * @param dht un pointeur vers le pointeur sur le debut de la liste de hashs
 *        (dht peut etre modifie si le hash de debut de liste est supprime).
 * @return le temps minimal avant qu'un autre hash ne soit obsolete.
*/
int gestion_obsolescence(l_hash **dht)
{
    l_hash *table_actu, *table_prec, *table_next;
    l_emplacement *emp_actu, *emp_next, *emp_prec;
    long int temps_actuel;
    int next_time;
    
    next_time = TEMPS_OBSOLESCENCE;
    temps_actuel = time(NULL);
    
    table_prec=NULL;
    /* Parcours de la liste de hash */
    for(table_actu=*dht; table_actu!=NULL;)
    {
        emp_prec=NULL;
        /* Parcours de la liste des adresses ip associees au hash */
        for(emp_actu=table_actu->dispo; emp_actu!=NULL;)
        {
            /* Si l'adresse ip est obsolete, on la supprime */
            if(temps_actuel-emp_actu->obsolescence > TEMPS_OBSOLESCENCE)
            {
                if(emp_prec==NULL)
                    table_actu->dispo = emp_actu->next;
                else
                    emp_prec->next = emp_actu->next;
                emp_next = emp_actu->next;
                free(emp_actu->adresse);
                free(emp_actu);
                emp_actu=emp_next;
            }
            else
            {
                /* Sinon, on regarde le temps qui lui reste avant
                   d'etre obsolete */
                if(TEMPS_OBSOLESCENCE+emp_actu->obsolescence-temps_actuel 
                                                                    < next_time)
                {
                    next_time = TEMPS_OBSOLESCENCE-temps_actuel
                                +emp_actu->obsolescence;
                }
                
                emp_prec=emp_actu;
                emp_actu=emp_actu->next;
            }
        }
        
        /* Si le hash ne contient plus d'adresse associee, on le supprime */
        if(table_actu->dispo==NULL)
        {
            if(table_prec==NULL)
                *dht = table_actu->next;
            else
                table_prec->next = table_actu->next;
                
            table_next=table_actu->next;
            free(table_actu->hash);
            free(table_actu);
            table_actu=table_next;            
        }
        else
        {
            table_prec = table_actu;
            table_actu=table_actu->next;
        }
    }
    
    return next_time+1;
}

/**
 *
 *
 *
 *
*/
int reception_transfert(message *m, l_hash **dht, l_serveur **st)
{
    int err=0;
    struct sockaddr *serveur;
    taille taille_addrlent;
    
    /* Si le message contient un serveur */
    if(message_get_s(m, &serveur, &taille_addrlent)==0)
    {
        err=add_a_serveurs(st, serveur, (socklen_t) taille_addrlent);
    }
    else
    {
        /* Reception d'un couple hash/adresse */
        err=serveur_put(m, dht, NULL, NULL);
    }
    
    return err;
}

/**
 * @brief Informe tout les serveurs connus de la creation d'un nouveau serveur.
 *
 * @param sockfd l'identifiant du socket a utiliser.
 * @param st un pointeur vers le debut de la liste de serveur.
 * @param nouveau_serv les informations du nouveau serveur.
 * @param addrlen la longueur des infos du nouveau serveur.
 * @return 0 en cas de reussite, un code d'erreur sinon.
*/
int informer_connexion_serveur(int sockfd, l_serveur *st, 
                        struct sockaddr * nouveau_serv, taille addrlen)
{
    int err;
    message *m;

    /* Creer un nouveau message de type transfert */
    err=create_message(&m, 't', SIZEOF_ENTETE);
    if(err!=0)
        return err;
    
    /* Ajout des donnees d'un serveur au message */
    err=add_data(m, 's', addrlen, nouveau_serv);
    if(err!=0)
    {
        delete_message(m);
        return err;
    }

    prepare_message(m);
    
    /* Envoie du message a tt les serveurs connus */
    for(; st!=NULL; st=st->next)
    {
        if(sendto(sockfd, m->contenu, m->lg_message, 0,
                  st->serveur, st->addrlen) == -1)
        {
            perror("Error sendto");
            delete_message(m);
            return 17;
        }
    }
    
    delete_message(m);
    
    return 0;
}

/**
 * @brief Simule un serveur stockant une table de hashage.
 *
 * Le serveur peut se lancer en solo, ou alors se lancer en se connectant a un
 * autre serveur afin de se partager la table de hash.
 *
 * @param argv[1] IP sa propre adresse ip.
 * @param argv[2] PORT port sur lequel ecouter.
 * @param argv[3] IP (facultatif) l'ip d'un serveur auquel se connecter.
 * @param argv[4] PORT (si argv[3] specifie) le port du serveur a contacter.
*/
int main(int argc, char **argv)
{
    int sockfd, sockfd2, err, last = 0;
    long int derniere_verification, temps_ecoule, next_time;
    struct addrinfo *head, *valide;
    message *m, *m2;
    l_hash *dht = NULL;
    l_serveur *st = NULL;
    socklen_t addrlen = sizeof(sockaddr_in);
    sockaddr_in client = {0};
    struct itimerval timer = {{SERVEUR_CHK_A_SEC,SERVEUR_CHK_A_MICROSEC},
                              {SERVEUR_CHK_A_SEC,SERVEUR_CHK_A_MICROSEC}};

    if((err=gestion_signaux())!=0)
    {
        return err;
    }
    
    /* Teste la validite de la ligne de commande */
    if(argc == 5) /* Cas d'une connexion a un autre serveur */
    {
        /* Creer un nouveau message de type new server */
        err=create_message(&m, 'n', SIZEOF_ENTETE);
        if(err!=0)
        {
            exit(err);
        }
        
        prepare_message(m);
        
        /*Initialisation de l'ecoute du serveur */
        err=get_addr(SERVEUR, argv[1], argv[2], &sockfd, NULL, NULL);
        if(err!=0)
        {
            delete_message(m);
            last=1;
        }

        /* Recuperation d'une adresse valide pour contacter le serveur
           auquel se connecter */
        err=get_addr(CLIENT, argv[3], argv[4], &sockfd2, &head, &valide);
        if(err!=0)
        {
            delete_message(m);
            close(sockfd);
            exit(err);
        }
        close(sockfd2);
        
        /* Envoie l'information de connexion a l'autre serveur */
        if(sendto(sockfd, m->contenu, m->lg_message, 0,
                  valide->ai_addr, valide->ai_addrlen) == -1)
        {
            perror("Error sendto");
            close(sockfd);
            freeaddrinfo(head);
            delete_message(m);
            exit(16);
        }
        
        delete_message(m);
        
        /* Reception du message */
        while(last==0)
        {
            err = recevoir_message(&m2, sockfd, NULL, NULL);
            if(err!=0)
            {
                close(sockfd);
                freeaddrinfo(head);
                exit(err);
            }
            
            /* Reception d'un element (hash/adresse ou serveur) */
            if(m2->type=='t')
            {
                err=reception_transfert(m2, &dht, &st);
                if(err!=0)
                {
                    delete_message(m2);
                    freeaddrinfo(head);
                    close(sockfd);
                    delete_l_hash(dht);
                    exit(err);
                }
            }
            else if(m2->type=='f') /* Fin de la mise a jour des donnees */
            {
                last=1;
            }
            delete_message(m2);
        }
        err=add_a_serveurs(&st, valide->ai_addr, valide->ai_addrlen);
        freeaddrinfo(head);
    }
    else if(argc == 3)  /*Cas de la creation d'un serveur solitaire */
    {
        /*Initialisation de l'ecoute du serveur */
        err=get_addr(SERVEUR, argv[1], argv[2], &sockfd, NULL, NULL);
        if(err!=0)
        {
            return err;
        }
    }
    else /* Cas de commande invalide */
    {
        printf("Usage : %s IP PORT\n", argv[0]);
        printf("Usage : %s IP PORT IP_AUTRE_SERVEUR PORT_AUTRE_SERVEUR\n", 
                                                                       argv[0]);
        exit(13);
    }

    /* Timer pour indiquer qu'il faut verifier si les serveurs sont toujours
       en vie */
    setitimer(ITIMER_REAL, &timer, NULL);
    
    /* Variables permettant de verifier quand il faut, si les donnees sont
       obsoletes ou non */
    temps_ecoule = 0;
    derniere_verification = time(NULL);
    next_time = TEMPS_OBSOLESCENCE;
    
    while(serveur_actif)
    {
        /* Si le delais d'attente des reponse des keep-alive est ecoule */
        if(check_K_A)
        {
            err=check_send_KA(&st,sockfd);
            if(err!=0)
                break;
            
            check_K_A=FALSE;
        }
        
        /* Attend l'arrivee d'un message */
        err=recevoir_message(&m, sockfd, (struct sockaddr *) &client, &addrlen);
        if(err!=0)
        {
            if(err==CODE_INTERRUP_SYSTEM)
            {
                /* Si l'interruption a eu lieu a cause de SIGALRM */
                if(check_K_A)
                    continue;
                /* Si l'interruption a eu lieu a cause de SIGINT */
                if(!serveur_actif)
                    break;
            }
            else
            {
                break;
            }
        }
        
        /* Avant de traiter le message, l'obsolescence des
           donnees peut etreverifiee selon le temps qui est 
           passe depuis la derniere verification */
        temps_ecoule = time(NULL)-derniere_verification;
        if(temps_ecoule>=next_time)
        {
            next_time = gestion_obsolescence(&dht);
            derniere_verification = time(NULL);
        }

        /* Effectue un action en fonction du type du message */
        switch(m->type)
        {
            /* Lit le message et stocke les donnees recues (put d'un hash) */
            case 'p':
                err=serveur_put(m, &dht, &st, &sockfd);
                printf("Arrivee Hash\n");
                if(err!=0)
                    serveur_actif = FALSE;
                break;
            
            /* Lit le message et recherche dans le DHT toutes les donnees
               voulues (get d'un hash) */
            case 'g':
                err=serveur_get(&m2, m, dht);
                if(err!=0)
                {
                    serveur_actif = FALSE;
                    break;
                }
                
                prepare_message(m2);
                
                /* Envoie la reponse au client */
                if(sendto(sockfd, m2->contenu, m2->lg_message, 0,
                            (struct sockaddr *) &client, addrlen) == -1)
                {
                    perror("Error sendto");
                    serveur_actif = FALSE;
                    err = 14;
                }
                delete_message(m2);
                break;
                
            /* Un nouveau serveur souhaite se connecter */
            case 'n':
                printf("Nouvelle connexion\n");
                /* Envoie la table de hashage et la liste de serveurs au serveur
                   se connectant */
                err=serveur_send_all(sockfd, (struct sockaddr *) &client,
                                     addrlen, dht, st);
                if(err!=0)
                {
                    serveur_actif = FALSE;
                    break;
                }
                
                /* Envoie le nouveau serveur a tt les serveurs connus */
                err=informer_connexion_serveur(sockfd, st,
                                (struct sockaddr *) &client, (taille)addrlen);
                if(err!=0)
                {
                    serveur_actif = FALSE;
                    break;
                }
                
                /* Ajoute le serveur dans la liste de serveurs */
                err=add_a_serveurs(&st,(struct sockaddr *) &client, addrlen);
                if(err!=0)
                    serveur_actif = FALSE;
                
                break;

            /* Un serveur informe qu'il s'arrete */
            case 'd':
                printf("Deconnexion d'un serveur\n");
                delete_server(&st,(struct sockaddr *) &client);
                break;

            /* Reception d'un message demandant si le serveur est
               toujours actif (keep-alive) */
            case 'k':
                /* Creer un nouveau message de type alive */
                err=create_message(&m2, 'a', SIZEOF_ENTETE);
                if(err!=0)
                {
                    serveur_actif = FALSE;
                    break;
                }
                
                prepare_message(m2);
                
                /* Envoie la reponse au serveur */
                if(sendto(sockfd, m2->contenu, m2->lg_message, 0,
                            (struct sockaddr *) &client, addrlen) == -1)
                {
                    perror("Error sendto");
                    serveur_actif = FALSE;
                    err = 15;
                }
                
                delete_message(m2);
                break;

                /*Reception de la reponse d'un serveur a un keep-alive */
            case 'a':
                isAlive(st, (struct sockaddr*) &client);
                break;
            case 't':
                /* Reception d'une donnée d'un autre serveur */
                err=reception_transfert(m, &dht, &st);
                if(err!=0)
                    serveur_actif = FALSE;
                break;
            /* Cas de message inconnu. Le message n'est pas pris en compte. */
            default:
                fprintf(stderr, "Type de message inconnu (%c)\n", m->type);
        }
        delete_message(m);
    }
    
    /* Informe les serveurs connus de l'arret de celui-ci */
    err=informer_arret_serveur(st,sockfd);
    printf("Fermeture du serveur\n");
    close(sockfd);
    delete_l_hash(dht);
    delete_l_serveurs(st);

    return err;
}



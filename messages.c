#include "messages.h"

/**
 * @brief Creer un nouveau message.
 *
 * Allocation d'une nouvelle structure pour un message.
 * Verification de la longueur a allouer.
 * Allocation de l'espace pour le contenu du messsage.
 * Initialisation de la structure.
 *
 * @param m un pointeur vers un pointeur sur un message
 *        (valeur de retour par effet de bord).
 * @param type le type du message.
 * @param lg le nombre d'octets a allouer pour le message.
 * @return 0 en cas de reussite, un code d'erreur sinon.
*/
int create_message(message **m, donnees type, unsigned int lg)
{
    message *m2 = malloc(sizeof(message));
    if(m2==NULL)
    {
        perror("Error malloc");
        return 50;
    }
    
    if(lg<SIZEOF_ENTETE)
        lg=SIZEOF_ENTETE;
    
    if(lg>MAX_MESS_SIZE)
        lg = MAX_MESS_SIZE;
        
    m2->contenu = malloc(lg);
    if(m2->contenu==NULL)
    {
        perror("Error malloc");
        free(m2);
        return 51;
    }
    
    m2->contenu[0] = type;
    m2->type = type;
    m2->lg_allouee = lg;
    m2->lg_message = SIZEOF_ENTETE;

    *m = m2;

    return 0;
}


/**
 * @brief Creer un message apres lecture d'une entete.
 *
 * La fonction recupere la taille stockee dans l'entete passee en argument,
 * puis appel la creation d'un message en lui passant la taille et le type.
 *
 * @param m un pointeur vers un pointeur sur un message
 *        (valeur de retour par effet de bord).
 * @param header une chaine de caractere representant l'entete du message.
 * @param 0 en cas de reussite, un code d'erreur sinon.
*/
int create_message_with_header(message **m, donnees *header)
{
    taille taille_message = *(taille *)(header+1);

    return create_message(m, header[0], taille_message);
}

/**
 * @brief Libere l'espace alloue par un message.
 *
 * @param m un pointeur sur un message dont l'uitilisation est terminee.
*/
void delete_message(message *m)
{
    if(m!=NULL)
    {
        if(m->contenu!=NULL)
            free(m->contenu);
        free(m);
    }
}

/**
 * @brief Ajoute un bloc dans un message.
 *
 * Realloue de l'espace s'il en manque.
 * Copie le type, la taille et les donnees dans le message.
 *
 * @param m un pointeur sur le message a remplir.
 * @param type le type de la donnee a ajouter.
 * @param lg la taille des donnees a ajouter.
 * @param data un pointeur sur les donnees a ajouter.
 * @return 0 en cas de reussite, un code d'erreur sinon.
*/
int add_data(message *m, donnees type, taille lg, void* data)
{
    donnees *tmp_realloc;
    
    /*Si la taille actuelle plus ce qu'il faut ajouter depasse l'espace alloue*/
    if(m->lg_message+lg+SIZEOF_ENTETE_BLOC > m->lg_allouee)
    {
        m->lg_allouee = m->lg_message+lg+SIZEOF_ENTETE_BLOC;
        
        if(m->lg_allouee > MAX_MESS_SIZE)
        {
            fprintf(stderr,"La taille du message est "\
                           "trop grande ( > %d)\n",MAX_MESS_SIZE);
            return 52;
        }
        
        tmp_realloc=realloc(m->contenu, m->lg_allouee);
        if(tmp_realloc==NULL)
        {
            perror("Error realloc");
            return 53;
        }
        m->contenu = tmp_realloc;
    }
    
    /* Ajout du type */
    m->contenu[m->lg_message] = type;
    m->lg_message+=SIZEOF_TYPE_BLOC;
    
    /* Ajout de la taille */
    memcpy(m->contenu+m->lg_message, &lg, SIZEOF_TAILLE_BLOC);
    m->lg_message+=SIZEOF_TAILLE_BLOC;
    
    /* Ajout des donnees */
    memcpy(m->contenu+m->lg_message, data, lg);
    m->lg_message+=lg;
    
    return 0;
}

/** 
 * @brief Ecrit la taille totale du message dans l'entete du message.
 *
 * @param m un pointeur sur le message complet.
*/
void prepare_message(message *m)
{
    memcpy(m->contenu+1, &(m->lg_message), 2);
}

/**
 * @brief Receptionne un message.
 *
 * Lit l'entete d'un message en attente d'etre lu, cree une structure pouvant
 * acceuillir le message, puis lit la totalitee du message.
 *
 * @param retour un pointeur vers un pointeur sur un message
 *        (valeur de retour par effet de bord).
 * @param sfd l'identifiant de la socket.
 * @param client une structure pouvant contenir les informations de l'emetteur
 *        (valeur de retour par effet de bord).
 * @param addrlen un entier pouvant contenir la taille de l'adresse
 *        de l'emetteur (valeur de retour par effet de bord).
 * @return 0 en cas de reussite, un code d'erreur sinon.
*/
int recevoir_message(message **retour, int sfd, 
                      struct sockaddr *client, socklen_t *addrlen)
{
    int taille_lue, err;
    donnees entete[SIZEOF_ENTETE];
    message *m;
    
    /* Lit l'entete du message tout en le gardant dans la file des messages
       a lire */
    if(recvfrom(sfd, entete, SIZEOF_ENTETE, MSG_PEEK, client, addrlen) == -1)
    {
        /* Interruption system (suppose SIGINT ou SIGALRM) */
        if(errno==EINTR)
            return CODE_INTERRUP_SYSTEM;
        /* Interruption programmee */
        if(errno==EAGAIN || errno==EWOULDBLOCK)
            return CODE_CANCEL_WAIT;
        perror("Error recvfrom");
        return 54; 
    }
    
    /* Cree un nouveau message a partir de l'entete lue */
    err=create_message_with_header(&m, entete);
    if(err!=0)
    {
        return err;
    }
    
    /* Lit l'ensemble du message */
    if((taille_lue=recvfrom(sfd, m->contenu, m->lg_allouee,
                                 0, client, addrlen)) == -1)
    {
        perror("recvfrom");
        delete_message(m);
        return 55;
    }
    
    m->lg_message=taille_lue;
    
    *retour = m;

    return 0;
}

/**
 * @brief Lit un message et renvoie le hash suivant trouve.
 *
 * Si l'adresse d'un message est specifiee, alors les variables static sont
 * initialisees et la recherche commence au debut de ce message. Si aucun
 * message n'est specifie alors la recherche continue la ou la lecture
 * precedente s'etait terminee.
 *
 * @param m un pointeur sur le message a lire ou 
          NULL pour continuer la lecture d'un message.
 * @param emplacement un pointeur sur une chaine de caractere
 *        (valeur de retour par effet de bord).
 * @param taille_lue taille de la donnee trouvee 
 *        (valeur de retour par effet de bord).
 * @return 0 si une donnee a ete trouvee, -1 sinon.
*/
int message_get_h(message *m, donnees** emplacement, taille *taille_lue)
{
    taille taille_element;
    static donnees *current = NULL;
    static donnees *fin_message = NULL;
    
    if(m!=NULL)
    {
        current = m->contenu+SIZEOF_ENTETE;
        fin_message = m->contenu+m->lg_message;
    }
    
    if(current==NULL || fin_message==NULL)
    {
        return -1;
    }
    
    /* Tant qu'il reste au moins l'entete d'un bloc a lire */
    while(current+SIZEOF_ENTETE_BLOC < fin_message)
    {
        taille_element=*(taille *)(current+SIZEOF_TYPE_BLOC);
        
        if(current[0]=='h')
        {
            *emplacement = current+SIZEOF_ENTETE_BLOC;
            *taille_lue = taille_element;
            current+=taille_element+SIZEOF_ENTETE_BLOC;
            return 0;
        }
        
        current+=taille_element+SIZEOF_ENTETE_BLOC;
    }
    
    return -1;
}

/**
 * @brief Lit un message et renvoie l'adresse suivante trouvee.
 *
 * Si l'adresse d'un message est specifiee, alors les variables static sont
 * initialisees et la recherche commence au debut de ce message. Si aucun
 * message n'est specifie alors la recherche continue la ou la lecture
 * precedente s'etait terminee.
 *
 * @param m un pointeur sur le message a lire ou 
          NULL pour continuer la lecture d'un message.
 * @param emplacement un pointeur sur une chaine de caractere
 *        (valeur de retour par effet de bord).
 * @param taille_lue taille de la donnee trouvee 
 *        (valeur de retour par effet de bord).
 * @return 0 si une donnee a ete trouvee, -1 sinon.
*/
int message_get_a(message *m, donnees** emplacement, taille *taille_lue)
{
    taille taille_element;
    static donnees *current = NULL;
    static donnees *fin_message = NULL;
    
    if(m!=NULL)
    {
        current = m->contenu+SIZEOF_ENTETE;
        fin_message = m->contenu+m->lg_message;
    }
    
    if(current==NULL || fin_message==NULL)
    {
        return -1;
    }
    
    /* Tant qu'il reste au moins l'entete d'un bloc a lire */
    while(current+SIZEOF_ENTETE_BLOC < fin_message)
    {
        taille_element=*(taille *)(current+SIZEOF_TYPE_BLOC);

        if(current[0]=='a')
        {
            *emplacement = current+SIZEOF_ENTETE_BLOC;
            *taille_lue = taille_element;
            current+=taille_element+SIZEOF_ENTETE_BLOC;
            return 0;
        }
        
        current+=taille_element+SIZEOF_ENTETE_BLOC;
    }
    
    return -1;
}

/**
 * @brief Lit un message et renvoie le serveur suivant trouve.
 *
 * Si l'adresse d'un message est specifiee, alors les variables static sont
 * initialisees et la recherche commence au debut de ce message. Si aucun
 * message n'est specifie alors la recherche continue la ou la lecture
 * precedente s'etait terminee.
 *
 * @param m un pointeur sur le message a lire ou 
          NULL pour continuer la lecture d'un message.
 * @param serveur un pointeur vers un pointeur sur une structure contenant les
 *        infos d'un serveur (valeur de retour par effet de bord).
 * @param taille_lue taille de la donnee trouvee 
 *        (valeur de retour par effet de bord).
 * @return 0 si une donnee a ete trouvee, -1 sinon.
*/
int message_get_s(message *m, struct sockaddr **serveur, taille *taille_lue)
{
    taille taille_element;
    static donnees *current = NULL;
    static donnees *fin_message = NULL;
    
    if(m!=NULL)
    {
        current = m->contenu+SIZEOF_ENTETE;
        fin_message = m->contenu+m->lg_message;
    }
    
    if(current==NULL || fin_message==NULL)
    {
        return -1;
    }
    
    /* Tant qu'il reste au moins l'entete d'un bloc a lire */
    while(current+SIZEOF_ENTETE_BLOC < fin_message)
    {
        taille_element=*(taille *)(current+SIZEOF_TYPE_BLOC);

        if(current[0]=='s')
        {
            *serveur = (struct sockaddr *)(current+SIZEOF_ENTETE_BLOC);
            *taille_lue = taille_element;
            current+=taille_element+SIZEOF_ENTETE_BLOC;
            return 0;
        }
        
        current+=taille_element+SIZEOF_ENTETE_BLOC;
    }
    
    return -1;
}

/**
 * @brief Recherche parmis toute les adresses possibles une adresse valide.
 *
 * Recupere les adresse Internet de l'adresse ou du nom de domaine passe en
 * parametre pour ensuite pouvoir l'assigner a un socket si c'est le serveur
 * qui demande, ou juste connaitre l'adresse du serveur si c'est le client qui
 * demande.
 *
 * @param role un entier representant le SERVEUR ou le CLIENT.
 * @param adresse la chaine de caractere designant l'adresse de destination dans
 *        le cas d'un client, ou l'adresse d'ecoute dans le cas d'un serveur.
 * @param port le numero du port a utiliser.
 * @param sockfd l'identifiant du socket (valeur de retour par effet de bord).
 * @param debut pointeur vers le pointeur sur le premier element de la liste
          chainee d'adresse possible (valeur de retour par effet de bord).
 * @param valide pointeur vers le pointeur sur la premiere adresse valide pour
          l'echange de donnees (valeur de retour par effet de bord).
 * @return 0 en cas de succes, un code d'erreur sinon.
*/
int get_addr(int role, char *adresse, char *port, int *sockfd, 
                    struct addrinfo **debut, struct addrinfo **valide)
{
    int err;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    
	hints.ai_flags = AI_PASSIVE;        /* For wildcard IP address */
	hints.ai_family = AF_UNSPEC;        /* AF_UNSPEC pour ipv4 et ipv6 */
	hints.ai_socktype = SOCK_DGRAM;     /* Datagrames */
	hints.ai_protocol = IPPROTO_UDP;    /* Protocole UDP */
	hints.ai_addr = NULL;
	hints.ai_canonname = NULL;
	hints.ai_next = NULL;
    
    /* Recupere les differentes Internet address possibles */
    if((err=getaddrinfo(adresse, port, &hints, &result))!=0)
    {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		return 56;
    }
    
    /* Teste les differentes adresses fournies jusqu'a en trouver une valable */
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		*sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (*sockfd == -1)
			continue;
	    
	    if(role==CLIENT)
	        break;
	    
	    /* Si c'est une demande du serveur et que bind fonctionne */
	    if(role==SERVEUR && 
	       bind(*sockfd, (struct sockaddr *) rp->ai_addr, rp->ai_addrlen)!=-1)
		{
			break;
	    }

		close(*sockfd);
	}
    
    /* Si aucune adresse n'est valide. */
	if (rp == NULL)
	{
		fprintf(stderr, "No DNS results\n");
		close(*sockfd);
		freeaddrinfo(result);
		return 57;
	}
	
	/* Retour des valeurs par effet de bord, si necessaire */
	if(debut==NULL)
	{
	    freeaddrinfo(result);
	}
	else
	{
	    *valide = rp;
	    *debut = result;
	}
	
	return 0;
}

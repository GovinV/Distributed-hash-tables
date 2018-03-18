#include "stockage_serveur.h"

/**
 * @brief Libere recursivement la memoire attribuee a une liste d'emplacement.
 *
 * Libere dans l'ordre : l'emplacement suivant, la chaine de caractere
 * representant une adresse, puis enfin l'emplacement actuel.
 *
 * @param emp le pointeur sur la liste chainee d'emplacement a liberer.
*/
void delete_l_emplacement(l_emplacement *emp)
{
    if(emp==NULL)
        return;
    
    delete_l_emplacement(emp->next);
    free(emp->adresse);
    free(emp);
}

/**
 * @brief Creer une nouvelle structure l_emplacement et l'initialise.
 *
 * Alloue de la place pour une strucuture l_emplacement puis pour une chaine de
 * caractere, puis initialise la valeur de l'adresse et la taille de l'adresse.
 *
 * @param retour un pointeur vers le pointeur dans lequel stocker l'adresse de
 *        la structure allouee (valeur de retour par effet de bord).
 * @param adresse une chaine de caractere representant une adresse IP.
 * @param taille_adresse la taille de la chaine de caractere adresse.
 * @return 0 en cas de reussite, un code d'erreur sinon
*/
int new_emplacement(l_emplacement **retour, donnees *adresse, 
                        taille taille_adresse)
{
    l_emplacement *emp = malloc(sizeof(l_emplacement));
    if(emp == NULL)
    {
        perror("Error malloc");
        return 101;
    }
    
    emp->adresse = malloc(taille_adresse);
    if(emp->adresse == NULL)
    {
        perror("Error malloc");
        free(emp);
        return 102;
    }
    
    memcpy(emp->adresse, adresse, taille_adresse);
    emp->taille_adresse = taille_adresse;
    emp->obsolescence = time(NULL);
    emp->next = NULL;
    
    *retour = emp;
    
    return 0;    
}

/**
 * @brief Ajoute un emplacement (une adresse IP) a une liste d'emplacement.
 *
 * Si la liste passee en parametre est NULL, alors un premier element est cree,
 * sinon on regarde si l'adresse n'est pas deja sockee dans la liste, et si elle
 * n'y est pas alors on l'ajoute a la fin.
 *
 * @param debut un pointeur vers le pointeur de debut de la liste des
 *        emplacements (*debut est modifie si *debut==NULL).
 * @param adresse la chaine representant l'adresse IP associee au hash.
 * @param taille_adresse la longueur de la chaine adresse.
 * @return 0 en cas de reussite, un code d'erreur sinon.
*/
int add_emplacement(l_emplacement **debut, donnees* adresse, 
                        taille taille_adresse)
{
    int err;
    l_emplacement *last, *emp;
    
    last = emp = *debut;

    /* Cas d'une liste vide */
    if(emp==NULL)
    {
        err=new_emplacement(debut, adresse, taille_adresse);
        return err;
    }
    
    /* Parcours de la liste */
    for(;emp!=NULL;emp=emp->next)
    {
        if(emp->next!=NULL)
            last = emp->next;
        
        /* Si l'adresse est deja associee a ce meme hash */
        if(taille_adresse==emp->taille_adresse &&
           strncmp((char*)adresse, (char*)emp->adresse, taille_adresse)==0)
        {
            emp->obsolescence = time(NULL);
            return 0;
        }
    }
    
    /* Cas d'ajout en fin de liste */
    err=new_emplacement(&last->next, adresse, taille_adresse);
    return err;
}

/**
 * @brief Libere recursivement la memoire attribuee la liste de hash.
 *
 * Libere dans l'ordre : Le hash suivant dans la liste, la liste d'emplacement
 * associee au hash actuel, la chaine de caractere representant le hash, et pour
 * finir la structure representant le hash actuel.
 *
 * @param table le pointeur sur la liste de hash a liberer.
*/
void delete_l_hash(l_hash* table)
{
    if(table==NULL)
        return;
    
    delete_l_hash(table->next);
    delete_l_emplacement(table->dispo);
    free(table->hash);
    free(table);
}

/**
 * @brief Creer une nouvelle structure l_hash et l'initialise.
 *
 * Alloue de l'espace pour la strucuture puis pour la chaine contenant le hash,
 * puis initialise les valeurs de la structure.
 * 
 * @param retour un pointeur vers le pointeur dans lequel stocker l'adresse de
 *        la structure allouee (valeur de retour par effet de bord).
 * @param hash la chaine representant le hash a stocker.
 * @param taille_hash la longueur de la chaine hash.
 * @param adresse la chaine representant une adresse IP associee au hash.
 * @param taille_adresse la longueur de la chaine adresse.
 * @return 0 en cas de reussite, un code d'erreur sinon.
*/
int new_hash(l_hash **retour, donnees* hash, taille taille_hash, 
                donnees* adresse, taille taille_adresse)
{
    int err;
    
    l_hash *table = malloc(sizeof(l_hash));
    if(table == NULL)
    {
        perror("Error malloc");
        return 103;
    }
    
    table->hash = malloc(taille_hash);
    if(table->hash == NULL)
    {
        perror("Error malloc");
        free(table);
        return 104;
    }
    
    memcpy(table->hash, hash, taille_hash);
    table->taille_hash = taille_hash;
    table->next = NULL;
    table->dispo = NULL;
    err=add_emplacement(&table->dispo, adresse, taille_adresse);
    if(err!=0)
    {
        free(table->hash);
        free(table);
        return err;
    }
    
    *retour = table;
    
    return 0;
}

/**
 * @brief Ajoute un hash a la liste des hash repertories par le serveur.
 *
 * Si la liste est vide, un premier element est cree (et la variable debut est
 * modifiee), sinon si le hash est deja present, on ajoute la nouvelle adresse
 * ip associee a sa liste d'adresse ip, et si il n'existe pas encore dans la
 * liste alors il est ajoute a la fin.
 *
 * @param debut un pointeur vers le pointeur sur le debut de la liste de hash.
 * @param hash une chaine representant la valeur du hash a stocker.
 * @param taille_hash la longueur de la chaine hash.
 * @param adresse une chaine representant une adresse associee au hash.
 * @param taille_adresse la longueur de la chaine adresse.
 * @return 0 en cas de reussite, un code d'erreur sinon.
*/
int add_hash(l_hash **debut, donnees* hash, taille taille_hash, 
                donnees* adresse, taille taille_adresse)
{
    int err;
    l_hash *last, *table;
    
    last = table = *debut;
    
    /* Cas d'une liste vide */
    if(table==NULL)
    {
        err=new_hash(debut, hash, taille_hash, adresse, taille_adresse);
        return err;
    }
    
    /* Parcours de la liste */
    for(;table!=NULL;table=table->next)
    {
        if(table->next!=NULL)
            last = table->next;
            
        /* Si le hash est deja present dans la liste */
        if(taille_hash==table->taille_hash &&
           strncmp((char*)hash, (char*)table->hash, taille_hash)==0)
        {
            err=add_emplacement(&table->dispo, adresse, taille_adresse);
            return err;
        }
    }
    
    /* Cas d'ajout en fin de liste */
    err=new_hash(&last->next, hash, taille_hash, adresse, taille_adresse);
    return err;
}

/**
 * @brief Libere recursivement la memoire attribuee la liste de serveurs.
 *
 * Libere la memoire du serveur suivant, puis la memoire de l'adresse du
 * serveur courant et pour finir la structure du serveur courant.
 *
 * @param serveurs un pointeur sur le premier element de la liste de serveurs.
*/
void delete_l_serveurs(l_serveur *serveurs)
{
    if(serveurs==NULL)
        return;
    
    delete_l_serveurs(serveurs->next);
    free(serveurs->serveur);
    free(serveurs);
}

/**
 * @brief Creer une nouvelle structure l_serveur et l'initialise.
 *
 * Alloue de la place pour une strucuture l_serveur puis pour une structure
 * pouvant contenir les information pour contacter un serveur, et initialise
 * les donnees.
 *
 * @param retour un pointeur vers le pointeur dans lequel stocker l'adresse de
 *        la structure allouee (valeur de retour par effet de bord).
 * @param serveur un pointeur sur une structure contenant les donnees du serveur
 * @param addrlen la taille de la structure serveur.
 * @return 0 en cas de reussite, un code d'erreur sinon
*/
int new_a_serveurs(l_serveur **retour,
                    struct sockaddr* serveur,socklen_t addrlen)
{
    l_serveur *emp = malloc(sizeof(l_serveur));
    if(emp == NULL)
    {
        perror("Error malloc");
        return 105;
    }
    
    emp->serveur = malloc(addrlen);
    if(emp->serveur == NULL)
    {
        perror("Error malloc");
        free(emp);
        return 106;
    }

    memcpy(emp->serveur, serveur, addrlen);
    emp->addrlen = addrlen;
    emp->present = 1;
    emp->next = NULL;

    *retour = emp;
    
    return 0;    
}

/**
 * @brief Ajoute un serveur (une structure sockaddr) a une liste de serveurs.
 *
 * @param debut un pointeur vers le pointeur de debut de la liste des
 *        serveurs (*debut est modifie si *debut==NULL).
 * @param serveur un pointeur sur une structure contenant les donnees du serveur
 * @param addrlen la taille de la structure serveur.
 * @return 0 en cas de reussite, un code d'erreur sinon.
*/
int add_a_serveurs(l_serveur **debut,struct sockaddr* serveur,socklen_t addrlen)
{
    int err;
    l_serveur *last, *emp;
    
    last = emp = *debut;

    /* Cas d'une liste vide */
    if(emp==NULL)
    {
        err=new_a_serveurs(debut, serveur, addrlen);
        return err;
    }
    
    /* Parcours de la liste */
    for(;emp!=NULL;emp=emp->next)
    {
        if(emp->next!=NULL)
            last = emp->next;
    }
    
    /*Ajout en fin de liste */
    err=new_a_serveurs(&last->next, serveur, addrlen);
    return err;
}

/**
 * @brief Supprime un serveur de la liste des serveurs.
 *
 * @param debut un pointeur vers un pointeur sur le debut de la liste de serveur
 * @param serveur les donnees caracterisant un serveur.
*/
void delete_server(l_serveur **debut, struct sockaddr* serveur)
{
    l_serveur *last, *emp;

    last = emp = *debut;

    if(emp==NULL)
    {
        return;
    }

    /* Si le serveur a supprimer est le premier element de la liste */
    if(sockaddrcmp(serveur,emp->serveur)==0)  
    {
        *debut=emp->next;
        free(emp->serveur);
        free(emp);
        return;
    }
    
    emp=emp->next;
    /* Parcours de la liste a la recherche du serveur a supprimer */
    for(;emp!=NULL;emp=emp->next)
    {      
        if(sockaddrcmp(serveur,emp->serveur)==0)  
        {
            last->next=emp->next;
            free(emp->serveur);
            free(emp);
            break;
        }
        last=emp;
    }
}

/**
 * @brief Compare les ports de deux structures sockaddr.
 *
 * @param x une premiere struct sockaddr.
 * @param y une deuxieme struct sockaddr.
 * @return 1 si les ports sont differents, 0 sinon.
*/
int sockaddr_cmp(struct sockaddr *x, struct sockaddr *y)
{
#define CMP(a, b) if (a != b) return 1

    CMP(x->sa_family, y->sa_family);

    if (x->sa_family == AF_INET) {
        struct sockaddr_in *xin = (void*)x, *yin = (void*)y;
        CMP(ntohs(xin->sin_port), ntohs(yin->sin_port));
    } else if (x->sa_family == AF_INET6) {
        struct sockaddr_in6 *xin6 = (void*)x, *yin6 = (void*)y;
        CMP(ntohs(xin6->sin6_port), ntohs(yin6->sin6_port));
    } else {
        return 1;
    }

#undef CMP
    return 0;
}

/**
 * @brief Recupere l'adresse d'une struct sockaddr dans une chaine de caractere.
 *
 * Recupere l'adresse ip sous forme de chaine de caractère en s'adaptant
 * a l'ipv4/ipv6.
 *
 * @param sa une structure dont on souhaite extraire l'adresse ip.
 * @param s chaine qui contiendra l'adresse ip.
 * @param maxlen taille disponible pour la chaine s.
*/
void get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen)
{
    switch(sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                    s, maxlen);
            break;

        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                    s, maxlen);
            break;

        default:
            strncpy(s, "Unknown AF", maxlen);
    }
}

/**
 * @brief Compare deux structure sockaddr contenant les informations de serveurs
 *
 * @param x une strcuture sockaddr.
 * @param y une deuxieme structure sockaddr.
 * @return 0 si les adresse ip et port sont identiques, 1 sinon.
*/
int sockaddrcmp(struct sockaddr *x, struct sockaddr *y)
{
    size_t adlen=128;
    if(sockaddr_cmp(x,y)==0)  
    {
        char ad1[128];
        char ad2[128];
        get_ip_str(x, ad1, adlen);
        get_ip_str(y, ad2, adlen);
        if(strlen(ad1)==strlen(ad2) && strncmp(ad1, ad2, strlen(ad1))==0)
            return 0;
    }
    return 1;
}

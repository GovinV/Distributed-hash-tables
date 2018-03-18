* Distributed hash table

Authors: Nicolas VERGNES and Govindaraj VETRIVEL

** Table of contents
    *** I/ Project's files
    *** II/ Data exchange format
        **** 1/ Message's global format
        **** 2/ Block's format inside Data blocks
    *** III/ Message's and data block's types
        **** 1/ Message's types
        **** 2/ Data block's types
    *** IV/ More


** I/ Project's files 

- client.c : client's code to send request to the server
             ( see `man ./man/client.1`)
             
- server.c : server's code stock and send data
             (see `man ./man/server.1`)

- messages.c : messages gestion code
               
- messages.h : header of messages.c

- stockage_serveur.c : data gestion
                       
- stockage_serveur.h : header stockage_serveur.c

- Makefile : makefile 

- man/client.1 : French man for client 

- man/server.1 : French man for server


** II/ Data exchange format

*** 1/ Message's global format

+----------+----------+--------------+
| TYPE_MSG | SIZE_MSG |     DATA     |
+----------+----------+--------------+

TYPE_MSG : 1 bytes for type
SIZE_MSG : 2 bytes for message's size
DATA : SIZE_MSG-3 bytes Data blocks

*** 2/ Block's format inside Data blocks

+-----------+-----------+-----------+
| TYPE_BLOC | SIZE_BLOC | DATA_BLOC |
+-----------+-----------+-----------+

Add as many time as needed.

TYPE_BLOC : 1 bytes for type 
SIZE_BLOC : 2 bytes for block's size
DATA_BLOC : SIZE_BLOC-3 bytes of data


** III/ Message's and data block's types

*** 1/ Message's types

- 'g' (get) : client ask data
- 'p' (put) : client put an information in the server
- 'r' (answer) : answer to a 'get'
- 'n' (new server) : new server connect to another one to have 
                     the different information
- 'f' (transfert end) : server notifies that he send everything
- 'k' (keep-alive) : between servers
- 'a' (alive) : answer to keep-alive
- 'd' (disconnection) : server notifies others of its end
- 't' (transfert) : server send data to another one

*** 2/ Data block's types

- 'a' (adress) : ip address
- 'h' (hash) 
- 's' (server) : structure with informations about a server


** IV/ More

- Linked lists to link address to a hash, and server lists
- Keep-alive
- data's obsolescence (30 seconds)


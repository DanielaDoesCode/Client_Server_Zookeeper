/********************************/
/*Cadeira de Sistemas Distribuídos
*Grupo 33
*Daniela Camarinha fc58199
*Rafael Ribeiro fc58193
*Pedro Piló fc58179
*********************************/
#include "inet.h"
#include "data.h"
#include "table.h"
#include "entry.h"
#include <signal.h>
#include <zookeeper/zookeeper.h>
#include "table_skel.h"
#include "network_server.h"



struct table_t *main_table;
int sockfd;

void sigint_handler(int signo) {
    table_destroy(main_table);
    table_skel_destroy(main_table);
    printf("Server Closed");
    exit(0);

}

int main(int argc, char **argv) {

    signal(SIGINT, sigint_handler);
    signal(SIGPIPE,SIG_IGN);


    if (argc != 4) {
        printf("Incorrect number of parameters\n");
        return -1;
    }

    int port = atoi(argv[1]);  
    int n_lists = atoi(argv[2]);
    char* zoo_address = (char*)malloc(sizeof(char*));
    zoo_address = argv[3];

    if ((sockfd = network_server_init(port)) == -1) {
        printf("Error initiating server\n");
        return -1;
    }

    main_table = table_skel_init(n_lists,zoo_address,port);
    if (main_table == NULL) {
        printf("Error creating server table\n");
        return -1;
    }

    while (1) {
        printf("Server ready, waiting for connections\n");
        int client_fd = network_main_loop(sockfd, main_table);
        if (client_fd == -1) {
            printf("Connection Lost\n");
        }
    }

    return 0;  
}
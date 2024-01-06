/********************************/
/*Cadeira de Sistemas Distribuídos
*Grupo 33
*Daniela Camarinha fc58199
*Rafael Ribeiro fc58193
*Pedro Piló fc58179
*********************************/
#include <pthread.h>
#include "table_skel.h"
#include "sdmessage.pb-c.h"
#include "client_stub.h"
#include "client_stub-private.h"
#include "inet.h"
#include "message-private.h"
#include "network_server.h"
#include "stats.h"

extern struct table_t *main_table;
extern struct statistics_t* stats;

int connsockfd;
struct sockaddr_in server, client;
int nbytes, count;
socklen_t size_client;
/* Função para preparar um socket de receção de pedidos de ligação
 * num determinado porto.
 * Retorna o descritor do socket ou -1 em caso de erro.
 */
int network_server_init(short port){
    //CRIAR SOCKET
    int sockfd;
    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0){ //cria socket de conexão
        printf("Erro ao criar o socket");
        return -1;
    }
    int opt = 1;
    int socket_option = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,&opt, sizeof(opt));
    if(socket_option == -1){
        printf("Error accepting socket option");
        return -1;
    }

    //PREENCHER ESTRUTURA SERVER COM ENDEREÇOS PARA ASSOCIAR
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port); // PORTA TCP
    server.sin_addr.s_addr = INADDR_ANY;
    // FAZ BIND
    if (bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0){ //faz bind para os dados anteriores
        printf("Erro ao fazer bind");
        close(sockfd);
        return -1;
    }

     return sockfd;

}

//handles the client thread part of the program
void* client_thread_handler(void* arg){
    stats->num_clients++;
    int connsockfd = *((int *)arg);
    MessageT* message= malloc(sizeof(MessageT));
    message_t__init(message);

     while((message = network_receive(connsockfd)) != NULL){
        //LER A MENSAGEM
        //ENTREGAR AO SKELETON
        //dentro do invoke é que se tem que verifica as condições do single writer - multi reader
        if(invoke(message, main_table) == -1){
            //printf("Error while invoking skeleton");
        }
        //ENVIAR RESPOSTA PARA O CLIENTE
        if(network_send(connsockfd,message) == -1){
            printf("Error while sending message to client");
        }

      }
      stats->num_clients--;
      close(connsockfd);
      pthread_exit(NULL); //isto vai ativar o join da main thread
}

/* A função network_main_loop() deve:
 * - Aceitar uma conexão de um cliente;
 * - Receber uma mensagem usando a função network_receive;
 * - Entregar a mensagem de-serializada ao skeleton para ser processada na tabela table;
 * - Esperar a resposta do skeleton;
 * - Enviar a resposta ao cliente usando a função network_send.
 * A função não deve retornar, a menos que ocorra algum erro. Nesse
 * caso retorna -1.
 */
int network_main_loop(int listening_socket, struct table_t *table){
    //ESTA É UMA SOCKET PARA OUVIR
    if (listen(listening_socket, 0) < 0){ //main thread faz isto
        printf("Error executing listen");
        close(listening_socket);
        return -1;
    }
    //MAIN LOOP DE LEITURA
    //quando dá accept, tem que criar uma thread
    pthread_t tid;

    while((connsockfd = accept(listening_socket,(struct sockaddr *) &client, &size_client))!= -1){
        printf("Client connection established\n");
        //cria thread que vai resolver o pedido do cliente
        if((pthread_create(&tid, NULL, &client_thread_handler, (void *)&connsockfd)) != 0){
            printf("Error creating thread\n");
            close(connsockfd);
            continue;
        }
        pthread_detach(tid);
    }
    printf("Error connecting to client");
    return -1;
}

MessageT *network_receive(int client_socket){
    short size;
    int bytes_read = read_all(client_socket, &size, sizeof(short));

    if (bytes_read != sizeof(short)) {
        //printf("Error reading message size");
        return NULL;
    }

    size = ntohs(size);

    void *message_read = malloc(size + 1);
    bytes_read = read_all(client_socket,message_read, size);

    if (bytes_read == -1) {
        printf("Error reading from client");
        free(message_read);
        return NULL;    
    }
    MessageT *msg = message_t__unpack(NULL, size, message_read);

    if(msg == NULL){
        printf("Error packing message");
        return NULL;
    }

    free(message_read);
    return msg;
    
}

int network_send(int client_socket, MessageT *msg){
    int len = message_t__get_packed_size(msg);
    short size_short = (short) len;
    size_short = htons(size_short);
    void *message_read = malloc(len);
    int serialize_res = message_t__pack(msg, message_read);

    if (serialize_res == -1){
        printf("Packing error");
        return -1;
    }
    int send_res = write_all(client_socket, &size_short, sizeof(short));
    if (send_res == -1){
        printf("Error sending to client");
        return -1;
    }

    send_res = write_all(client_socket, message_read, len);
    if (send_res == -1){
        printf("Error sending to client");
        return -1;
    }
    return 0;
}

int network_server_close(int socket){
	return close(socket);
}
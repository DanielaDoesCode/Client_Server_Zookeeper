/********************************/
/*Cadeira de Sistemas Distribuídos
*Grupo 33
*Daniela Camarinha fc58199
*Rafael Ribeiro fc58193
*Pedro Piló fc58179
*sha la la
*********************************/
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "client_stub-private.h"
#include "message-private.h"
#include "network_client.h"


/* Esta função deve:
 * - Obter o endereço do servidor (struct sockaddr_in) com base na
 *   informação guardada na estrutura rtable;
 * - Estabelecer a ligação com o servidor;
 * - Guardar toda a informação necessária (e.g., descritor do socket)
 *   na estrutura rtable;
 * - Retornar 0 (OK) ou -1 (erro).
 */
int network_connect(struct rtable_t *rtable){
    struct sockaddr_in server;
    int sockfd;
    int server_port = (rtable->server_port);
    char *server_adress = (rtable->server_address);


    // CRIA SOCKET TCP
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Erro ao criar socket TCP");
        free(server_adress); //unico char que precisa de ser liberto?
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(server_port); 

    if (inet_pton(AF_INET, server_adress, &server.sin_addr) < 1) { // Endereço IP
        printf("Erro ao converter IP\n");
        free(server_adress);
        return -1;
    }

    //CONECTAR A SOCKET EM SOCKFD AO SERVER DESCRITO EM SOCKADDR
    if(connect(sockfd,(struct sockaddr *)&server, sizeof(server)) < 0){
        printf("Erro ao conectar-se ao servidor");
        free(server_adress);
        return -1;
    }
    //CONNECTION SUCCESSFUL, STORING THE DESCRIPTOR
    rtable->sockfd= sockfd; //POG POG
    return 0;


}

/* Esta função deve:
 * - Obter o descritor da ligação (socket) da estrutura rtable_t;
 * - Serializar a mensagem contida em msg;
 * - Enviar a mensagem serializada para o servidor;
 * - Esperar a resposta do servidor;
 * - De-serializar a mensagem de resposta;
 * - Tratar de forma apropriada erros de comunicação;
 * - Retornar a mensagem de-serializada ou NULL em caso de erro.
 */
MessageT* network_send_receive(struct rtable_t *rtable, MessageT *msg){
    int sockfd = rtable->sockfd;
    int len = message_t__get_packed_size(msg);
    short size_short = (short)len;
    uint8_t* buf = malloc(len);
    message_t__pack(msg, buf);

    if (buf == NULL) {
        printf("Memory allocation error");
        free(buf);
        message_t__free_unpacked(msg, NULL);
        return NULL;
    }

    uint8_t* size_buffer = malloc(sizeof(short));
    if (size_buffer == NULL) {
        printf("Memory allocation error");
        free(buf);
        free(size_buffer);
        message_t__free_unpacked(msg, NULL);
        return NULL;
    }

    size_short = htons(size_short);
    memcpy(size_buffer, &size_short, sizeof(short));

    // Send the size of the buffer
    if (write_all(sockfd, size_buffer, sizeof(short)) != sizeof(short) ||
        write_all(sockfd, buf, len) != len) {
        printf("Error sending data to the server");
        free(buf);
        free(size_buffer);
        message_t__free_unpacked(msg, NULL);
        return NULL;
    }

    // Receive the size of the response buffer
    short response_size;
    if (read_all(sockfd, &response_size, sizeof(short)) != sizeof(short)) {
        printf("Error receiving response size from the server");
        free(buf);
        free(size_buffer);
        message_t__free_unpacked(msg, NULL);
        return NULL;
    }

    len = ntohs(response_size);
    uint8_t* response_buf = malloc(len);

    if (response_buf == NULL) {
        printf("Memory allocation error");
        free(buf);
        free(size_buffer);
        message_t__free_unpacked(msg, NULL);
        free(response_buf);
        return NULL;
    }

    // Receive the response buffer
    if (read_all(sockfd, response_buf, len) != len) {
        printf("Error receiving data from the server");
        free(buf);
        free(size_buffer);
        free(response_buf);
        message_t__free_unpacked(msg, NULL);
        return NULL;
    }

    // Deserialize the response
    MessageT* response_msg = message_t__unpack(NULL, len, response_buf);

    // Clean up allocated memory
    free(buf);
    free(size_buffer);
    free(response_buf);

    if (response_msg == NULL) {
        printf("Error unpacking response message");
        free(response_msg);
        return NULL;
    }
    //printf("WEEEEEE");
    return response_msg;

}


int network_close(struct rtable_t *rtable){
    int sockfd = rtable->sockfd;
    return close(sockfd);

}
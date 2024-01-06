/********************************/
/*Cadeira de Sistemas Distribuídos
*Grupo 33
*Daniela Camarinha fc58199
*Rafael Ribeiro fc58193
*Pedro Piló fc58179
*********************************/
#include "data.h"
#include "entry.h"
#include "table.h"
#include "client_stub-private.h"
#include "client_stub.h"
#include "inet.h"
#include "sdmessage.pb-c.h"
#include "network_client.h"
#include "stats.h"
#include "string.h"


/* Remote table, que deve conter as informações necessárias para comunicar
 * com o servidor. A definir pelo grupo em client_stub-private.h
 */
struct rtable_t;

/* Função para estabelecer uma associação entre o cliente e o servidor, 
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna a estrutura rtable preenchida, ou NULL em caso de erro.
 */
struct rtable_t *rtable_connect(char *address_port){
    puts("Entra no rtable_connect");
    struct rtable_t *table = (struct rtable_t *)malloc(sizeof(struct rtable_t));

    if (table == NULL) {
        printf("Error allocating memory for rtable");
        return NULL;
    }

    char hostname[256];
    int port;

    if (sscanf(address_port, "%255[^:]:%d", hostname, &port) != 2) {
        printf("Invalid address_port format.\n");
        free(table);
        free(address_port);
        return NULL;
    }

    table->server_port = port;
    table->server_address = strdup(hostname); 
    printf("%s\n", table->server_address);
    printf("%d\n", table->server_port);

    if (table->server_address == NULL) {
        printf("Error allocating memory for server_address\n");
        free(table->server_address);
        free(table);
        free(address_port);
        return NULL;
    }

    if(network_connect(table) == -1){
        free(table);
        free(address_port);
        return NULL;
    }
    printf("retorna tabela?\n");
    printf("address table -> %s\n",table->server_address);
    printf("port -> %d\n",table->server_port);
    return table;
    

}

/* Termina a associação entre o cliente e o servidor, fechando a 
 * ligação com o servidor e libertando toda a memória local.
 * Retorna 0 se tudo correr bem, ou -1 em caso de erro.
 */
int rtable_disconnect(struct rtable_t *rtable){
    puts("ENtra no disconnect\n");
    close(rtable->sockfd);
    free(rtable->server_address);
    free(rtable);
    if(rtable == NULL){
        return 0;
    }
    return -1;
}

/* Função para adicionar um elemento na tabela.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Retorna 0 (OK, em adição/substituição), ou -1 (erro).
 */
int rtable_put(struct rtable_t *rtable, struct entry_t *entry){

    MessageT *msg = malloc(sizeof(MessageT));
    if (msg == NULL) {
        printf("Error initializing message");
        free(msg);
        return -1;
    }

    message_t__init(msg);

    EntryT new_entry;
    entry_t__init(&new_entry);

    new_entry.key = strdup(entry->key);
    new_entry.value.data = malloc(entry->value->datasize);
    
    if (new_entry.key == NULL || new_entry.value.data == NULL) {
        printf("Error initializing new_entry");
        free(msg);
        free(new_entry.key);
        free(new_entry.value.data);
        entry_t__free_unpacked(&new_entry, NULL);
        return -1;
    }

    memcpy(new_entry.value.data, entry->value->data, entry->value->datasize);
    new_entry.value.len = entry->value->datasize;

    msg->opcode = 10;
    msg->c_type = 10;
    msg->entry = &new_entry;

    MessageT *response = network_send_receive(rtable, msg);

    
    if (response->opcode != 11) {
        printf("Error on network side");
        free(msg);
        free(new_entry.key);
        free(new_entry.value.data);
        message_t__free_unpacked(response, NULL);
        entry_t__free_unpacked(&new_entry, NULL);
        return -1;
    }
    return 0;


}

/* Retorna o elemento da tabela com chave key, ou NULL caso não exista
 * ou se ocorrer algum erro.
 */
struct data_t *rtable_get(struct rtable_t *rtable, char *key){
    MessageT *msg = malloc(sizeof(MessageT));
    message_t__init(msg);
    if (msg == NULL) {
        printf("Error initializing message");
        free(msg);
        return NULL;
    }

    msg->opcode = 20;
    msg->c_type = 20;
    msg->key = key;

    MessageT *response = network_send_receive(rtable, msg);
    if (response == NULL || response->opcode != 21) {
        free(msg);
        return NULL;
    }

    void *data_dup = malloc(response->value.len);
    if (data_dup == NULL) {
        printf("Error allocating memory for data");
        message_t__free_unpacked(response, NULL);
        free(msg);
        return NULL;
    }

    memcpy(data_dup, response->value.data, response->value.len);
    struct data_t *data = data_create(response->value.len, data_dup);
    message_t__free_unpacked(response, NULL);
    free(msg);

    return data; 
}

/* Função para remover um elemento da tabela. Vai libertar 
 * toda a memoria alocada na respetiva operação rtable_put().
 * Retorna 0 (OK), ou -1 (chave não encontrada ou erro).
 */
int rtable_del(struct rtable_t *rtable, char *key){
    MessageT *msg = malloc(sizeof(MessageT));
    message_t__init(msg);
    if (msg == NULL){
        printf("Error initializing message");
        free(key);
        free(msg);
        return -1;
    }

    msg->opcode = 30;
    msg->c_type = 20;
    msg->key = key;
    MessageT *response = network_send_receive(rtable, msg);
    
    if(response->opcode != 31){
        //printf("Error on network side");
        free(msg);
        free(response);
        return -1;
    }

    return 0;
}

/* Retorna o número de elementos contidos na tabela ou -1 em caso de erro.
 */
int rtable_size(struct rtable_t *rtable){
    MessageT *msg = malloc(sizeof(MessageT));
    message_t__init(msg);
    if (msg == NULL){
        printf("Error initializing message");
        free(msg);
        return -1;
    }
    msg->opcode = MESSAGE_T__OPCODE__OP_SIZE;
    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
    MessageT *response = network_send_receive(rtable, msg);
    if (response->opcode != 41){
        printf("Error on network side");
        free(msg);
        free(response);
        return -1;
    }
    return response->result;
}

/* Retorna um array de char* com a cópia de todas as keys da tabela,
 * colocando um último elemento do array a NULL.
 * Retorna NULL em caso de erro.
 */
char **rtable_get_keys(struct rtable_t *rtable){
    MessageT *msg = malloc(sizeof(MessageT));
    message_t__init(msg);
    if (msg == NULL){
        printf("Error initializing message");
        free(msg);
        return NULL;
    }
    msg->opcode = 50;
    msg->c_type = 70;
    MessageT *response = network_send_receive(rtable, msg);
    if(response->opcode != 51){
        printf("Error on network side");
        free(msg);
        free(response);
        return NULL;
    }

    char** key_array = (char**)malloc((response->n_keys + 1) * sizeof(char*));

    for(int i = 0; i < response->n_keys; i++){
        key_array[i] = strdup(response->keys[i]);
    }
    key_array[response->n_keys] = NULL;
    return key_array;
}

/* Liberta a memória alocada por rtable_get_keys().
 */
void rtable_free_keys(char **keys) {
    table_free_keys(keys);
}

/* Retorna um array de entry_t* com todo o conteúdo da tabela, colocando
 * um último elemento do array a NULL. Retorna NULL em caso de erro.
 */
struct entry_t **rtable_get_table(struct rtable_t *rtable){
    MessageT *msg = malloc(sizeof(MessageT));;
    message_t__init(msg);
    if (msg == NULL){
        printf("Error initializing message");
        free(msg);
        return NULL;
    }
    msg->opcode = 60;
    msg->c_type = 70;
    MessageT *response = network_send_receive(rtable, msg);
    if(response->opcode != 61){
        printf("Error on network side");
        free(msg);
        free(response);
        return NULL;
    }
    
    struct entry_t **new_entries = malloc((response->n_entries + 1)*sizeof(struct entry_t));
    if(new_entries == NULL){
        printf("error alocating memory to entries");
        free(new_entries);
        return NULL;
    }

    for(int i = 0; i < response->n_entries; i++){
        struct data_t *temp_data = data_create(response->entries[i]->value.len, response->entries[i]->value.data);
        struct entry_t *temp = entry_create(response->entries[i]->key, temp_data);
        new_entries[i] = temp;
    } 
    new_entries[response->n_entries] = NULL;
    return new_entries;
    
}

/* Obtém as estatísticas do servidor. 
*/
struct statistics_t* rtable_stats(struct rtable_t *rtable) {
    MessageT *msg = malloc(sizeof(MessageT));
    message_t__init(msg);
    if (msg == NULL){
        printf("Error initializing message");
        free(msg);
        return NULL;
    }
    msg->opcode = 70;
    msg->c_type = 70; 
    MessageT *response = network_send_receive(rtable, msg);
    if(response->opcode != 71){
        printf("Error on network side");
        free(msg);
        free(response);
        return NULL;
    }
    struct statistics_t *statistics = malloc(sizeof(struct statistics_t));
    statistics->num_clients = response->stats->clients;
    statistics->num_ops = response->stats->num_ops;
    statistics->op_time = response->stats->time;
    return statistics;
    
}

/* Liberta a memória alocada por rtable_get_table().
 */
void rtable_free_entries(struct entry_t **entries){
    for (int i = 0;entries[i] != NULL; i++) {
        entry_destroy(entries[i]);
    }
    free(entries);
}
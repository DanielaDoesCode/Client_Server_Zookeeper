/********************************/
/*Cadeira de Sistemas Distribuídos
*Grupo 33
*Daniela Camarinha fc58199
*Rafael Ribeiro fc58193
*Pedro Piló fc58179
*********************************/
#include <pthread.h>
#include <zookeeper/zookeeper.h>
#include <ifaddrs.h>
#include <zoo_skel.h>
#include <netdb.h>
#include "table.h"
#include "client_stub-private.h"
#include "client_stub.h"
#include "network_client.h"
#include "sdmessage.pb-c.h"
#include "table_skel.h"
#include "entry.h"
#include "data.h"
#include "inet.h"
#include "stats.h"
#include "sys/time.h"

#define ZDATALEN 1024 * 1024

struct statistics_t *stats;
pthread_mutex_t lock_data = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

pthread_mutex_t lock_stats = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t stats_cond = PTHREAD_COND_INITIALIZER;

int readers_count = 0;
int writers_count = 0;
int stats_readers = 0;


//*Zookeeper elements
static zhandle_t *zh;
char* host_ip;
const char* zoo_adress_info;
static char node_name[1024] = "";
int number_order_id;
typedef struct String_vector zoo_string;
zoo_string* children_list;
const char* root_path = "/chain";
static struct rtable_t *successor_metadata;


// ZooKeeper event watcher function for the connection to the zookeeper service
static void watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
    //isto é o que ele faz quando há alguma alteração na ligação
    if(type == ZOO_CHANGED_EVENT){
        printf("Error with connection...disconnecting");
        rtable_disconnect(successor_metadata);
        zookeeper_close(zh);
        }
}



/* Inicia o skeleton da tabela.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke(). O parâmetro n_lists define o número de listas a
 * serem usadas pela tabela mantida no servidor.
 * Retorna a tabela criada ou NULL em caso de erro.
 */
struct table_t *table_skel_init(int n_lists, char* zoo_address, int port){

    //* Setting up addresses and ports------------------------------------------------------------------
    host_ip = get_host_address();
    strcat(host_ip, ":");
    char str_port[20]; // Ensure enough space to hold the converted string
    sprintf(str_port, "%d", port); // Convert int to string
    strcat(host_ip,str_port);



    //*1 - Zookeeper connection ------------------------------------------------------------------------
    zh = zookeeper_init(zoo_address,watcher,100000, 0, 0, 0);
	if (zh == NULL)	{
		fprintf(stderr, "Error connecting to ZooKeeper server!\n");
	    exit(EXIT_FAILURE);
	}

    if (ZNONODE == zoo_wexists(zh, root_path, watcher, NULL, NULL)) {
				if (ZOK == zoo_create( zh, root_path, NULL, -1, & ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0)) {
					fprintf(stderr, "%s created!\n", root_path);
				} else {
					fprintf(stderr,"Error Creating %s!\n", root_path);
					exit(EXIT_FAILURE);
				} 
    }
    puts("viu que root node existe");
    
    //* 2- Creating this server's node -----------------------------------------------------------------
    int flags = ZOO_EPHEMERAL | ZOO_SEQUENCE;
    char construct_path[120] = "";
	strcat(construct_path,root_path); 
	strcat(construct_path,"/node");
	int new_path_len = 1024;
	char* new_path = malloc(new_path_len);

    //criar com a conexão estabelecida com o handle zh e o nome do node vai para new_path
    if(ZOK != zoo_create(zh, construct_path, host_ip, strlen(host_ip), & ZOO_OPEN_ACL_UNSAFE, flags | ZOO_CREATE_OP, new_path, new_path_len)){
        printf("Erro creating znode from path!");
        return NULL;
    }

    puts("criou o seu próprio node");

    //guardar o nome do novo node criado
    strcpy(node_name, new_path);
    number_order_id = atoi(strrchr(node_name, 'e') + 1);


    //*3 - Obter a lista dos filhos de /chain-----------------------------------------------------------
    children_list =	(zoo_string *) malloc(sizeof(zoo_string));
    if (ZOK != zoo_wget_children(zh, root_path, child_watcher, 0, children_list)) {
				fprintf(stderr, "Error setting watch at %s!\n", root_path);
                return NULL;
			}

    puts("obteu lista dos filhos");        

    //*4 - Fazer watch dos filhos do server -------------------------------------------------------------
    for(int i = 0; i < children_list->count; i++){
        char node_path[256];
        snprintf(node_path, sizeof(node_path), "%s/%s", root_path, children_list->data[i]);
        if(ZOK != zoo_wexists(zh, node_path, child_watcher, NULL, NULL)){
            fprintf(stderr, "Error setting watch on node %s!", node_path);
            return NULL;
        }
    }

    puts("fez watch dos filhos"); 

    //* 5 - Conectar ao sucessor  ----------------------------------------------------------------------- 
    char* successor_node = get_successor(children_list); //isto retorna a address do server e port do server
    if(successor_node != NULL){
        successor_metadata = rtable_connect(successor_node); //cria rtable dentro do rtable connect e faz logo a conexão
    }

    puts("conectou ao sucessor"); 
    
    //* 7 - conectar ao antecessor ----------------------------------------------------------------------


    struct table_t *table = table_create(n_lists);
    stats = malloc(sizeof(struct statistics_t));
    if(table == NULL){
        printf("Error initializing table");
        return NULL;
    }

     puts("criou a tabela"); 


    char* antecessor_node = get_anteccessor(children_list);
    
    if(antecessor_node != NULL){
        struct rtable_t* antecessor_node_connection = rtable_connect(antecessor_node);
        //tabela temporária
        struct entry_t **replicate_table = rtable_get_table(antecessor_node_connection);
        rtable_disconnect(antecessor_node_connection);

        //* 8 - Atualizar a tabela local com a tabela do antecessor -----------------------------------------
    for(int i = 0; replicate_table[i] != NULL; i++){
        table_put(table,replicate_table[i]->key, replicate_table[i]->value);
    }

    //*Dar free á tabela temporária e retornar a tabela local
    rtable_free_entries(replicate_table);

    puts("deu retrieve da tabela ao antecessor");
    }

    puts("deu retrieve da tabela ao antecessor"); 


    //*retorna tabela local
    return table;
}

/* Liberta toda a memória ocupada pela tabela e todos os recursos 
 * e outros recursos usados pelo skeleton.
 * Retorna 0 (OK) ou -1 em caso de erro.
 */
int table_skel_destroy(struct table_t *table){
    table_destroy(table);
    if(table != NULL){
        return 1;
    }
    return 0;
}

/* Executa na tabela table a operação indicada pelo opcode contido em msg 
 * e utiliza a mesma estrutura MessageT para devolver o resultado.
 * Retorna 0 (OK) ou -1 em caso de erro.
*/
int invoke(MessageT *msg, struct table_t *table){
    int operation = msg->opcode;
    int type = msg->c_type;
    int pre_exec;

    if (!(operation == 70 && type == 70)) {

        struct timeval pre_time;
        gettimeofday(&pre_time, NULL);
        //Tempo em microsegundos = segundos(tv_sec) * 1000000 + microsegundos(tv_usec)
        pre_exec = pre_time.tv_sec * 1000000 + pre_time.tv_usec;

    }

    // put <key> <data>
    if (operation == 10 && type == 10) {

        pthread_mutex_lock(&lock_data);
        while(readers_count > 0 || writers_count > 0){
            pthread_cond_wait(&condition, &lock_data);
        }

        writers_count++;
        pthread_mutex_unlock(&lock_data);

        char *new_key = malloc(strlen(msg->entry->key) + 1);
        strcpy(new_key, (msg->entry->key));
        
        

        void* data_dupped = malloc(msg->entry->value.len);
        memcpy(data_dupped, msg->entry->value.data, msg->entry->value.len);
        struct data_t *new_data = data_create(msg->entry->value.len, data_dupped);

        int result = table_put(table, new_key, new_data);
        if(successor_metadata != NULL){
            struct entry_t * zoo_entry = entry_create(new_key, new_data);
            rtable_put(successor_metadata, zoo_entry);
        }
        if (result == -1) {
            msg->opcode = 99;
            msg->c_type = 70;
            update_stats(pre_exec);
            pthread_mutex_lock(&lock_data);
            writers_count--;
            pthread_cond_broadcast(&condition);
            pthread_mutex_unlock(&lock_data);
            return -1;
        }

        msg->opcode = 11;
        msg->c_type = 70;

        update_stats(pre_exec);
        pthread_mutex_lock(&lock_data);
        writers_count--;
        pthread_cond_broadcast(&condition);
        pthread_mutex_unlock(&lock_data);
        return 0;

    // get <key>
    }else if (operation == 20 && type == 20) {

        readers_count++;

        struct data_t *new_data = table_get(table, msg->key);
        if (new_data == NULL) {
            msg->opcode = 99;
            msg->c_type = 70;
            update_stats(pre_exec);
            readers_count--;
            return -1;
        }
        msg->opcode = 21;
        msg->c_type = 30;

        void* data_dupped = malloc(new_data->datasize);
        memcpy(data_dupped, new_data->data, new_data->datasize);
        msg->value.data = data_dupped;
        msg->value.len =new_data->datasize;
        data_destroy(new_data);
        update_stats(pre_exec);

        readers_count--;
        pthread_cond_broadcast(&condition);
        return 0;

    // del <key>
    }else if (operation == 30 && type == 20) {

         pthread_mutex_lock(&lock_data);
        while(readers_count > 0 || writers_count > 0){
            pthread_cond_wait(&condition, &lock_data);
        }
        writers_count++;
        pthread_mutex_unlock(&lock_data);


        int result = table_remove(table, msg->key);
        if(successor_metadata != NULL){
            rtable_del(successor_metadata,msg->key);
        }
        if (result == -1 || result == 1) {
            msg->opcode = 99;
            msg->c_type = 70;
            update_stats(pre_exec);
            pthread_mutex_lock(&lock_data);
            writers_count--;
            pthread_cond_broadcast(&condition);
            pthread_mutex_unlock(&lock_data);
            return -1;
        }
        msg->opcode = 31;
        msg->c_type = 70;
        update_stats(pre_exec);

        pthread_mutex_lock(&lock_data);
            writers_count--;
        pthread_cond_broadcast(&condition);
        pthread_mutex_unlock(&lock_data);
        return 0;

    // size
    }else if (operation == 40 && type == 70) {
        readers_count++;

        int result = table_size(table);
        if (result == -1) {
            msg->opcode = 99;
            msg->c_type = 70;
            update_stats(pre_exec);
            readers_count--;
            pthread_cond_broadcast(&condition);
            return -1;
        }
        msg->opcode = 41;
        msg->c_type = 40;
        msg->result = result;
        update_stats(pre_exec);

        readers_count--;
        pthread_cond_broadcast(&condition);
        return 0;

    // get keys    
    }else if (operation == 50 && type == 70) {
        readers_count++;

        char **keys = table_get_keys(table);
        if (keys == NULL) {
            msg->opcode = 99;
            msg->c_type = 70;
            update_stats(pre_exec);
            readers_count--;
            pthread_cond_broadcast(&condition);
            return -1;
        }
        msg->opcode++;
        msg->c_type = 50;
        char **dup_keys = malloc(table_size(table) * sizeof(char*));

        if(dup_keys == NULL){
            printf("Error alocating memory");
            msg->opcode = 99;
            msg->c_type = 70;
            update_stats(pre_exec);
            readers_count--;
            pthread_cond_broadcast(&condition);
            return -1;
        }

        for(int i = 0; i< table_size(table); i++){
            dup_keys[i] = strdup(keys[i]);
        }

        msg->n_keys = table_size(table);
        msg->keys = dup_keys;
        table_free_keys(keys);
        update_stats(pre_exec);

        readers_count--;
        pthread_cond_broadcast(&condition);
        return 0;


    // get table
    }else if (operation == 60 && type == 70){

        readers_count++;

        EntryT** entries = malloc(table_size(table)* sizeof(EntryT*));
        char ** keys = table_get_keys(table);
        for(int i = 0; i < table_size(table); i++){
            struct data_t* data_temp = table_get(table, keys[i]);
            entries[i] = malloc(sizeof(EntryT));
            entry_t__init(entries[i]);
            entries[i]->key = strdup(keys[i]);
            entries[i]->value.len = data_temp->datasize;
            entries[i]->value.data = malloc(data_temp->datasize);
            memcpy(entries[i]->value.data, data_temp->data, data_temp->datasize);
        }
        msg->entries = entries;
        msg->n_entries = table_size(table);
        msg->opcode = 61;
        msg->c_type = 60;
        table_free_keys(keys);
        update_stats(pre_exec);

        readers_count--;
        pthread_cond_broadcast(&condition);
        return 0;
    
    //stats
    }else if (operation == 70 && type == 70){
        stats_readers++;

        if (stats == NULL) {
            printf("Error alocating memory");
            msg->opcode = 99;
            msg->c_type = 70;
            stats_readers--;
            return -1;
        }
        StatisticsT* message_stats = (StatisticsT*)malloc(sizeof(StatisticsT));
        statistics_t__init(message_stats);
        message_stats->clients = stats->num_clients;
        message_stats->num_ops= stats->num_ops;
        message_stats->time = stats->op_time;
        msg->stats = message_stats;
        msg->opcode = 71;
        msg->c_type = 80;
        stats_readers--;
        return 0;

    }else{ // operation or type are incorrect
        msg->opcode = 99;
        msg->c_type = 70;
        stats_readers--;
        return -1;
    }
}

void update_stats(int pre_exec) {

    pthread_mutex_lock(&lock_stats);
    while(stats_readers > 0){
        pthread_cond_wait(&stats_cond, &lock_stats);
    }
    pthread_mutex_unlock(&lock_stats);


    struct timeval pos_time;
    gettimeofday(&pos_time, NULL);
    int pos_exec = pos_time.tv_sec *1000000 + pos_time.tv_usec;

    stats->num_ops++;
    stats->op_time += pos_exec - pre_exec;
}

static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx){
    zoo_string *temp_list = (zoo_string*)malloc(sizeof(zoo_string));
    if(state == ZOO_CONNECTED_STATE) {
        if(type == ZOO_CHILD_EVENT){
            printf("Children of node %s have changed!\n", zpath);
            //Por causa nas possíveis mudanças, tentar dar fetch á nova lista atualizada
            if(ZOK != zoo_wget_children(zh, root_path, child_watcher, 0, temp_list)){
                printf("Error setting watch at %s!\n", root_path);
            }
            if(successor_metadata != NULL){
                rtable_disconnect(successor_metadata);
            }

            if (children_list != NULL) {
                free(children_list);
                children_list = NULL; // Ensure it's set to NULL after freeing
            }
            children_list = temp_list;

            char* new_successor = get_successor(temp_list); //getting the new successor
            printf("new successor -> %s\n", new_successor);
            printf("new successor\n");

            if(new_successor != NULL){
                printf("entra\n");
                successor_metadata = rtable_connect(new_successor);
                //rtable_disconnect já dá free a successor_metadata
            }
            fprintf(stderr,"connecting....\n");
        }
    }
    free(temp_list);
}

char* get_successor(zoo_string* children_list){
    puts("entrou em get successor");
    int max_id = number_order_id;
    char* successor_node = NULL;
    char* child_name = NULL;

    // encontrar o node que é o sucessor do atual
    for (int i = 0; i < children_list->count; i++) {
        child_name = children_list->data[i];
        printf("child_name -> %s", child_name);
        const char* numeric_part = strrchr(child_name, 'e');
        int node_id = atoi(numeric_part + 1); // Increment by 1 to skip '/'

        if (node_id > max_id) {
            max_id = node_id;
            successor_node = children_list->data[i];
        }
    }

    if (successor_node == NULL) {
        printf("No successor found.\n");
        return NULL;
    }

    int node_data_len = 100000;
    char* node_data = (char*)malloc(node_data_len * sizeof(char));
    if (node_data == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return NULL;
    }

    char* search_node = malloc(sizeof("/chain/") + 11);
    strcpy(search_node, "/chain/"); // Initialize search_node with the base string
    strcat(search_node, successor_node); // Concatenate node_data to search_node

    // Fetch node data - node_data will store the IP
    int data_len = node_data_len;
    int ret = zoo_wget(zh, search_node, child_watcher, 0, node_data, &data_len, NULL);
    if (ret != ZOK) {
        fprintf(stderr, "Error fetching node data for node %s or node does not exist!\n", successor_node);
        free(node_data); // Free allocated memory
        return NULL;
    }

    return node_data; // Return the IP:port of the successor node //this data is the ip:port of the sucessor
}

char* get_anteccessor(zoo_string* children_list){
    puts("entrou na função get antecessor");
    int lower_id = -1;
    char* antecessor_node = NULL;
    char* node_data = NULL;

    for (int i = 0; i < children_list->count; i++) {
        puts("entra no loop\n");
        char* child_name = children_list->data[i];
        const char* numeric_part = strrchr(child_name, 'e');
        int node_id = atoi(numeric_part + 1); // Increment by 1 to skip '/'

        if (node_id > lower_id && node_id < number_order_id) {
            lower_id = node_id;
            antecessor_node = children_list->data[i];
        }
    }
    puts("retira a lista dos filhos de zookeeper\n");
    if (antecessor_node == NULL) {
        printf("No antecessor found to replicate table from.\n");
        return NULL;
    }

    int node_data_len = 100000;
    node_data = (char*)malloc(node_data_len * sizeof(char));
    if (node_data == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return NULL;
    }


    char* search_node = malloc(sizeof("/chain/") + 11); // Assuming maximum size for search_node
    strcpy(search_node, "/chain/"); // Initialize search_node with the base string
    strcat(search_node, antecessor_node); // Concatenate node_data to search_node

    printf("search node -> %s\n", search_node);
    int data_len = node_data_len;
    int ret = zoo_wget(zh, search_node, child_watcher, 0, node_data, &data_len, NULL);
    if (ret != ZOK) {
        fprintf(stderr, "Error fetching node data for node %s!\n", antecessor_node);
        free(node_data); // Free allocated memory
        return NULL;
    }
    puts("retorna a node_data\n");
    printf("node_data -> %s\n", node_data);
    return node_data;
}


char* get_host_address(){
    struct ifaddrs *addrs;
    char suitableIP[1025];

    if (getifaddrs(&addrs) != 0) {
        printf("Error getting host ip address");
        return NULL;
    }

    while(addrs != NULL){
        if(addrs->ifa_addr != NULL && addrs->ifa_addr->sa_family == AF_INET){
            //strstr -> pesquisa por uma substring em ifa_name com eth ou wlan
            if (getnameinfo(addrs->ifa_addr, sizeof(struct sockaddr_in), suitableIP, 1025, NULL, 0, 1) != 0) {
                freeifaddrs(addrs);
                return NULL;
            }

            if (strcmp(suitableIP, "127.0.0.1") == 0) { //comparing with the loopback ip
                addrs = addrs->ifa_next;
                continue;
            }

            return strdup(suitableIP);
        }
        addrs = addrs->ifa_next;
    }
    freeifaddrs(addrs); // Free memory
    return NULL; // Return NULL if suitable IP address not found
}
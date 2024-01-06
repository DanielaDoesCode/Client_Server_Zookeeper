/********************************/
/*Cadeira de Sistemas Distribuídos
*Grupo 33
*Daniela Camarinha fc58199
*Rafael Ribeiro fc58193
*Pedro Piló fc58179
*********************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>
#include <zoo_client.h>
#include "client_stub.h"
#include "client_stub-private.h"
#include "data.h"
#include "entry.h"
#include "list.h"
#include "table.h"
#include "inet.h"
#include "stats.h"
#define ZDATALEN 1024 * 1024

//*Zookeeper elements
static zhandle_t *zh;
const char* root_path = "/chain";
char* head_addr;
char* tail_addr;
static char *watcher_ctx = "ZooKeeper Data Watcher";
struct rtable_t* head_metadata = NULL;
struct rtable_t* tail_metadata = NULL;
zoo_string* children_list;

//Zookeeper function
// ZooKeeper event watcher function
void watcher(zhandle_t *zzh, int type, int state, const char *path, void *context) {
    // Watcher function to handle events
    //isto é o que ele faz quando há alguma alteração
    if(type == ZOO_CHANGED_EVENT && state == ZOO_CONNECTED_STATE){
        printf("Data of node %s has changed\n", path);
        // TODO - o que fazer quando isto acontece
    }
}


void sigint_handler_client(int signo) {
    rtable_disconnect(head_metadata);
    rtable_disconnect(tail_metadata);
    printf("Client exited\n");
    exit(0);
}

int main(int argc, char **argv) {
    
   
    //*----------------------------------------Start--------------------------------------------
    if(argc != 2){
        printf("Invalid args! \n Usage: table-client <zookeeper_ip:zookeeper_port>\n");
        return -1;
    }

    char* zookeeper_address = argv[1];
    

    //*1 - Zookeeper connection ----------------------------------------------------------------------------------
    zh = zookeeper_init(zookeeper_address,watcher,2000, 0, 0, 0);
	if (zh == NULL)	{
		fprintf(stderr, "Error connecting to ZooKeeper server!\n");
	    exit(EXIT_FAILURE);
	}
     puts("inicia conexão ao zookeeper");

    if (ZNONODE == zoo_exists(zh, root_path, 0, NULL)) {
				if (ZOK == zoo_create( zh, root_path, NULL, -1, & ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0)) {
					fprintf(stderr, "%s created!\n", root_path);
				} else {
					fprintf(stderr,"Error Creating %s!\n", root_path);
					exit(EXIT_FAILURE);
				} 
    }

     puts("viu que root node existe");

    //*2 - Obter a lista dos filhos de /chain---------------------------------------------------------------------
    children_list =	(zoo_string *) malloc(sizeof(zoo_string));
    if (ZOK != zoo_wget_children(zh, root_path, child_watcher, watcher_ctx, children_list)) {
				fprintf(stderr, "Error setting watch at %s!\n", root_path);
                return -1;
	}

    puts("obtêm a lista dos filhos");

    //*3 - Obter o filho com menor id (head) e conectar-----------------------------------------------------------
    char* head_addr = get_lowest_id_node(children_list);
    if(head_addr != NULL){
        head_metadata = rtable_connect(head_addr);
    }
    puts("obter head node");


    //*4 - Obter o filho com maior id(tail) e conectar------------------------------------------------------------
    char* tail_addr = get_highest_id_node(children_list);
    if(head_addr != NULL){
        tail_metadata = rtable_connect(tail_addr);
    }
    puts("obter tail node");



    char *str = malloc(MAX_MSG);
     if (str == NULL) {
        printf("Error allocating memory for user input\n");
        rtable_disconnect(head_metadata);
        rtable_disconnect(tail_metadata);
        return -1;
    }

    while (1) {
        printf("Command:");
        if (fgets(str, MAX_MSG, stdin) != NULL) {
            if (str[strlen(str) - 1] == '\n') {
                str[strlen(str) - 1] = '\0';
            }
        }
        char* token = strtok(str, " ");

        if (token != NULL) {
            if (strcmp(token, "put") == 0 || strcmp(token, "p") == 0) {
                char* key = strtok(NULL, " ");
                char* data = strtok(NULL, " ");
                if(key == NULL || data == NULL){
                    printf("Invalid arguments. Usage: put <key> <value>\n");
                    continue;
                }

                char* data_dupped = malloc(strlen(data) + 1);
                memcpy(data_dupped, data, strlen(data));

                struct data_t* data_item = data_create(strlen(data), data_dupped);
                if (data_item == NULL) {
                    printf("Error creating data item\n");
                    continue;
                }
                struct entry_t *entry = entry_create(strdup(key), data_item);
                if (entry == NULL) {
                    printf("Error creating entry\n"); 
                    data_destroy(data_item);
                    continue;
                }

                int status = rtable_put(head_metadata, entry);

                if(status == -1){
                    printf("Error on command put\n");
                    entry_destroy(entry);
                    data_destroy(data_item);
                    continue;
                }

            }else if (strcmp(token, "get") == 0 || strcmp(token, "g") == 0) {
                char* key = strtok(NULL, " ");
                if(key == NULL){
                    printf("Invalid arguments. Usage: get <key>\n");
                    continue;
                }
                struct data_t* data_found = rtable_get(tail_metadata, key);
                if(data_found == NULL){
                    printf("Error on rtable_get or key not found!\n");
                    continue;
                }
                printf("%.*s\n",data_found->datasize,(char*)data_found->data);



            } else if (strcmp(token, "del") == 0 || strcmp(token, "d") == 0) {
                char* key = strtok(NULL, " ");
                if(key == NULL){
                    printf("Invalid arguments. Usage: del <key>\n");
                    continue;
                }
                int status = rtable_del(head_metadata,key);
                if(status == -1){
                    printf("Error on rtable_del or key not found!\n");
                    continue;
                }
                printf("Entry removed\n");
        
            } else if (strcmp(token, "size") == 0 || strcmp(token, "s") == 0) {
                int size = rtable_size(tail_metadata);
                if(size == -1){
                    printf("Error getting the size of the table\n");
                    continue;
                }
                printf("Table size: %d\n", size);

            } else if (strcmp(token, "getkeys") == 0 || strcmp(token, "k") == 0) {
                char ** keys = rtable_get_keys(tail_metadata);
                for(int i = 0; keys[i]!= NULL ; i++){
                    printf("%s\n", keys[i]);
                }

            } else if (strcmp(token, "gettable") == 0 || strcmp(token, "t") == 0) {
                struct entry_t **entries = rtable_get_table(tail_metadata);
                for(int i = 0; entries[i] != NULL; i++){
                    printf("%s :: %s\n", entries[i]->key, (char*)entries[i]->value->data);
                }
                rtable_free_entries(entries);

            } else if (strcmp(token, "stats") == 0) {
               struct statistics_t* statistics = rtable_stats(tail_metadata);
                printf("Número total de operações executadas pelo servidor: %d\n", statistics->num_ops);
                printf("Tempo total gasto na execução de operações da tabela: %d micro-segundos\n", statistics->op_time);
                printf("Número de clientes atualmente ligados ao servidor: %d\n", statistics->num_clients);

            } else if (strcmp(token, "quit") == 0 ||strcmp(token, "q") == 0 ){
                rtable_disconnect(head_metadata);
                rtable_disconnect(tail_metadata);
                printf("Client disconnected");
                exit(0);

            } else{
                printf("Invalid Command:\n");
                printf("Usage: p[ut] <key> <value> |g[et] <key> | d[el] <key> | s[ize] | [get]k[eys] | [get]t[able] | stats | q[uit]\n");
            }
        }
    }
}

static void child_watcher(zhandle_t *wzh, int type, int state, const char* zpath, void *watcher_ctx){
    zoo_string* children_list = (zoo_string *) malloc(sizeof(zoo_string));
    if(state == ZOO_CONNECTED_STATE) {
        if(type == ZOO_CHILD_EVENT){
            printf("Children of node %s have changed!\n", zpath);
            //Por causa nas possíveis mudanças, tentar dar fetch á nova lista atualizada
            if(ZOK != zoo_wget_children(zh, root_path, child_watcher, watcher_ctx, children_list)){
               char* head = get_lowest_id_node(children_list);
               if(strcmp(head, head_addr) != 0){
                rtable_disconnect(head_metadata);
                head_metadata = rtable_connect(head);
                strcpy(head_addr, head);
               }
               char* tail = get_highest_id_node(children_list);
               if(strcmp(tail, tail_addr) != 0){
                rtable_disconnect(tail_metadata);
                tail_metadata = rtable_connect(tail);
                strcpy(tail_addr, tail);
               }
                printf("Error setting watch at %s!\n", root_path);
            }
        }
    }
}


char* get_lowest_id_node(zoo_string* children_list){
    puts("Entra em get_lowest_id_node");
    int lowest_id = 100000;
    char* chosen_node = NULL;

    for (int i = 0; i < children_list->count; i++) {
        char* child_name = children_list->data[i];
        fprintf(stderr,"child_name -> %s\n", child_name);
        const char* numeric_part = strrchr(child_name, 'e');
        int node_id = atoi(numeric_part + 1); // Increment by 1 to skip 'e'
        
        if (node_id < lowest_id) {
            lowest_id = node_id;
            chosen_node = children_list->data[i];
            fprintf(stderr,"chosen_node -> %s\n", chosen_node);
        }
    }


    // Fetch node data - node_data will store the IP
    int node_data_len = 100000;
    char* node_data = (char*)malloc(node_data_len * sizeof(char) + 1);
    if (node_data == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return NULL;
    }

    char* search_node = malloc(sizeof("/chain/") + 11); // Assuming maximum size for search_node
    strcpy(search_node, "/chain/"); // Initialize search_node with the base string
    strcat(search_node, chosen_node); // Concatenate the numeric part of the child__node


    int data_len = node_data_len;
    int ret = zoo_wget(zh, search_node, child_watcher, NULL, node_data, &data_len, NULL);
    if (ret != ZOK) {
        fprintf(stderr, "Error fetching node data for node %s!\n", search_node);
        free(node_data); // Free allocated memory
        return NULL;
}
    puts("retorna node data");
    return node_data; //this data is the ip:port of the lowest id node
}

char* get_highest_id_node(zoo_string* children_list){
  int highest_id = -1;
    char* chosen_node = NULL;

    for (int i = 0; i < children_list->count; i++) {
        char* child_name = children_list->data[i];
        const char* numeric_part = strrchr(child_name, 'e');
        int node_id = atoi(numeric_part + 1); // Increment by 1 to skip '/'

        if (node_id > highest_id) {
            highest_id = node_id;
            chosen_node = children_list->data[i];
        }
    }

    // Fetch node data - node_data will store the IP
    int node_data_len = 100000;
    char* node_data = (char*)malloc(node_data_len * sizeof(char));
    if (node_data == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return NULL;
    }

    char* search_node = malloc(sizeof("/chain/") + 11); // Assuming maximum size for search_node
    strcpy(search_node, "/chain/"); // Initialize search_node with the base string
    strcat(search_node, chosen_node); // Concatenate node_data to search_node

    printf("search node: %s\n", search_node);
    int data_len = node_data_len;
    int ret = zoo_wget(zh, search_node, child_watcher, NULL, node_data, &data_len, NULL);
    if (ret != ZOK) {
        printf("Error fetching node data for node %s!\n", chosen_node);
        free(node_data); // Free allocated memory
        return NULL;
    }

    return node_data; //this data is the ip:port of the lowest id node
}


/********************************/
/*Cadeira de Sistemas Distribuídos
*Grupo 33
*Daniela Camarinha fc58199
*Rafael Ribeiro fc58193
*Pedro Piló fc58179
*********************************/

#include <stdlib.h>
#include <list.h>
#include <string.h>

#include "table.h"
#include "table-private.h"
#include "list.h"
#include "list-private.h"
#include "data.h"
#include "entry.h"



struct table_t *table_create(int n) {
    if(n == 0){
        return NULL;
    }
    struct table_t *table = (struct table_t*)malloc(sizeof(struct table_t));
    table->lists = (struct list_t**) malloc(sizeof(struct list_t)* n);
    if(table != NULL && table->lists != NULL){
        for(int i = 0; i < n; i++){
            table->lists[i] = list_create();
        }
        table->size = n;
        return table;
    }
    return table;
}

int table_destroy(struct table_t *table) {
    for(int i = 0; i < table->size; i++){
        list_destroy(table->lists[i]);
    }
        free(table->lists);
        free(table);
        return 0;

}

int table_put(struct table_t *table, char *key, struct data_t *value) {
    if(table == NULL || key == NULL || value == NULL){
        return -1;
    }

    int hash = hash_code(key, table->size);
    char *copy_key = (char*)malloc(strlen(key) + 1);
    strcpy(copy_key, key);
    struct data_t *copy_data = data_dup(value);
    struct entry_t *new_entry = entry_create(copy_key,copy_data);

    return list_add(table->lists[hash], new_entry);
        
    
    }

//without hash functions usage
struct data_t *table_get(struct table_t *table, char *key) {
    if(table == NULL || key == NULL){
        return NULL;
    }
    int index = 0;
    while(index < table->size){
        struct entry_t *target_entry = list_get(table->lists[index], key);
        if(target_entry != NULL){
            return data_dup(target_entry->value); //retornar uma cópia
        }
        index++;
    }
    return NULL;
}

int table_remove(struct table_t *table, char *key) {
    if(table == NULL || key == NULL){
        return -1;
    }
    int hash = hash_code(key, table->size);
    return list_remove(table->lists[hash], key);
}

int table_size(struct table_t *table) {
    if(table == NULL){
        return -1;
    }

    int size = 0;
    for(int i = 0; i < table->size; i++){
        size += table->lists[i]->size;
    }
    return size;
}

char **table_get_keys(struct table_t *table){
    if (table == NULL) {
    return NULL;
    }
    int index = 0;
    // nr de elementos na tabela * tamanho de char
    char **key_array = (char**)malloc((table_size(table) + 1 )* sizeof(char*));
    for (int i = 0; i < table->size; i++) {
        struct list_t *current_list = table->lists[i];
        char** list_keys = list_get_keys(current_list);
        for (int j = 0; j < list_size(current_list); j++) {
            key_array[index] = list_keys[j];
            index++;
        }
        free(list_keys);
    
    }
    key_array[index] = NULL; 
    return key_array;
    
}

int table_free_keys(char **keys) {
    int count = 0;
    while(keys[count]!=NULL){
        free(keys[count]);
        count++;
    }
    free(keys);
    return 0;
}

int hash_code(char *key, int n){
    int c = 0;
    for (int i = 0; key[i] != '\0'; i++)
        c += atoi(&key[i]);
    return c % n;
}
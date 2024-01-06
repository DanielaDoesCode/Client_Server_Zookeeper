/********************************/
/*Cadeira de Sistemas Distribuídos
*Grupo 33
*Daniela Camarinha fc58199
*Rafael Ribeiro fc58193
*Pedro Piló fc58179
*********************************/

#include <stdlib.h>
#include <data.h>
#include <string.h>
#include "entry.h"




struct entry_t *entry_create(char *key, struct data_t *data){
    if((key == NULL) || (data == NULL)){
        return NULL;
    }
    struct entry_t *new_entry = (struct entry_t*)malloc(sizeof(struct entry_t));
    if(new_entry != NULL){
        new_entry->key = key;
        new_entry->value = data;
        return new_entry;
    }
    return NULL;
}


int entry_destroy(struct entry_t *entry){
    if(entry == NULL){
        return -1;
    }
    if((entry->key == NULL) || (entry->value== NULL)){
        return -1;
    }else{
        data_destroy(entry->value);
        free(entry->key);
        free(entry);
        return 0;
    }
}


struct entry_t *entry_dup(struct entry_t *entry){
    if(entry == NULL){
        return NULL;
    }
    if(entry->key ==  NULL || entry->value == NULL){
        return NULL;
    }

    struct entry_t *new_entry = malloc(sizeof(struct entry_t));
    if(new_entry == NULL){
        return NULL;
    }
    new_entry->value = data_dup(entry->value);
    new_entry->key = strdup(entry->key);
    return new_entry;
}



int entry_replace(struct entry_t *entry, char *new_key, struct data_t *new_value){
    if(entry == NULL || new_key == NULL || new_value == NULL){
        return -1;
    }else if(entry->key == NULL || entry->value == NULL){
        return -1;
    }else{
        data_destroy(entry->value);
        free(entry->key);
        entry->key = new_key;
        entry->value = new_value;
        return 0;
    }  
}


int entry_compare(struct entry_t *entry1, struct entry_t *entry2){
    if(entry1 == NULL || entry2 == NULL || entry1->key == NULL || entry2->key == NULL){
        return -2;
    }else{
        int strcm = strcmp(entry1->key, entry2->key);
        return strcm == 0 ? 0 : ((strcm > 0) ? 1 : -1);
    }
}

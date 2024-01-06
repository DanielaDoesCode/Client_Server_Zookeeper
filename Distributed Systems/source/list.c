/********************************/
/*Cadeira de Sistemas Distribuídos
*Grupo 33
*Daniela Camarinha fc58199
*Rafael Ribeiro fc58193
*Pedro Piló fc58179
*********************************/

#include <stdlib.h>
#include <string.h>
#include "entry.h"
#include "list.h"
#include "list-private.h"



struct node_t *node_create(struct entry_t *entry){
    struct node_t *node = malloc(sizeof(struct node_t));
    if(node != NULL){
        node->entry = entry;
        node->next = NULL;
        return node;
    }
    return node;
}



int node_destroy(struct node_t *node){
    if(node == NULL){
        return -1;
    }
    entry_destroy(node->entry);
    free(node);
    return 0;
}

struct list_t *list_create(){
    struct list_t *list = malloc(sizeof(struct list_t));
    if(list != NULL){
        list->head = NULL;
        list->size = 0;
        return list;
    }
    return list;
}


int list_destroy(struct list_t *list){
    if(list == NULL){
        return -1;
    }
    struct node_t* current = list->head;
    while (current != NULL) {
        struct node_t* next = current->next;
        node_destroy(current);
        current = next;
    }
    free(list);
    return 0;
}


int list_add(struct list_t *list, struct entry_t *entry){
     if (list == NULL || entry == NULL) {
        return -1;
    }

    //quando lista está vazia
    if (list->head == NULL) {
        list->head = node_create(entry);//!
        list->size++;
        return 0;
    }

    //inicizalizar nodes para iterar
    struct node_t *currentNode = list->head;
    struct node_t *penultDummyNode = NULL;

    //enquanto current node não é null
    while (currentNode != NULL) { //enquanto não cheguei ao final da lista
        int comparison = entry_compare(currentNode->entry, entry);

        if (comparison == 0) {
            entry_destroy(currentNode->entry);
            currentNode->entry = entry;
            //free(newNode);
            //free(penultDummyNode); //não posso dar free a alguma coisa que esteja nula
            return 1;
        }

        //se o que esta na lista é maior do que o queremos meter
        if (comparison == 1) {
            struct node_t *newNode = node_create(entry);
            //isto significa que estamos na segunda posição a contar do inicio da lista
            //se queremos trocar de posicao o primeiro elemento da lista
            if (penultDummyNode == NULL) {
                newNode->next = currentNode;
                list->head = newNode; // no inicio
                free(penultDummyNode); //faz sentido pois não é usado.
                list->size++;
                return 0;
            }
                //se não for null, meaning que há mais um antes da head
                newNode->next = currentNode;
                penultDummyNode->next = newNode;
                list->size++;
                //usamos tudo pelo que nao podemos dar free
                return 0;
        } 


        if (comparison == -1) { //unica situação que nos temos que precaver no -1
            if (currentNode->next == NULL) { //significa que estamos no final da lista
                currentNode->next = node_create(entry); // Insert at the end
                list->size++;
                return 0;
            }
        }
            penultDummyNode = currentNode;
            currentNode = currentNode->next;
    }

    free(penultDummyNode);
    free(currentNode);
    //free(newNode);
    return -1;
}


int list_remove(struct list_t *list, char *key){
    if(list == NULL || key == NULL){ //checkar parametros
        return -1;
    }
    if(list->head == NULL){
        return 1;
    }
    struct node_t *target = list->head;
    struct node_t *beforeTarget = NULL;

    for(int i = 0; i<list->size; i++){
        int cmp = strcmp(target->entry->key, key);
        if(cmp == 0){
            //quando é no inicio da lista
            if(beforeTarget == NULL){
                list->head = target->next;
                list->size--;
                node_destroy(target); //eliminar o node
                free(beforeTarget);
                return 0;
            }else{
                beforeTarget->next = target->next;
                node_destroy(target);
                list->size--;
                return 0;
            }
        }
        beforeTarget = target;
        target = target->next;
    }
    return 1;

}


struct entry_t *list_get(struct list_t *list, char *key){
    if (list == NULL || key == NULL || list->size == 0 || 
        list->head == NULL) { //checkar parametros
        return NULL;
    }

    struct node_t *target = list->head;
    while (target != NULL) { //enquanto lista não terminar
        if (strcmp(target->entry->key, key) == 0) { //comparar key do target
            return target->entry;
        }

        target = target->next;
    }

    return NULL;

    
}


int list_size(struct list_t *list){
    if(list != NULL && list->size >= 0) //checkar parametros
        return list->size;
    return -1;
}


char **list_get_keys(struct list_t *list){
    if(list == NULL || list->size <= 0){
        return NULL;
    }
    //array de pointers
    char** keyArray = (char **)malloc((list->size + 1) * sizeof(char *));
    if(keyArray == NULL){
        return keyArray;
    }
    struct node_t* temp_node = list->head;
    int index = 0;
    while(index <(list->size)){
        keyArray[index] = (char*)malloc(sizeof(temp_node->entry->key));
        strcpy(keyArray[index], temp_node->entry->key);
        temp_node = temp_node->next;
        index++;
    }
    node_destroy(temp_node);
    keyArray[index] = NULL;
    return keyArray;
    
    
}


int list_free_keys(char **keys){
    if(keys == NULL){
        return -1;
    }

    for(int i  = 0; keys[i] != NULL; i++){
        free(keys[i]); //para cada key
    }

    free(keys);   //par dar free ao array de pointers completo
    return 0;
}

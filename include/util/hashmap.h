#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

typedef struct Node Node;
struct Node {
    char* key;
    void* item;
    Node* next;
};

void Node_free(Node n) {
    free(n.key);
    free(n.item);
    if(n.next != NULL) Node_free(*(n.next));
}

typedef struct {
    size_t size;
    size_t cap;
    size_t bytes_per_elem;
    Node** buckets;
} HashMap;

unsigned int HashMap_init(HashMap* hm, size_t cap, size_t bytes_per_elem) {
    hm->size = 0;
    hm->cap = cap;
    hm->bytes_per_elem = bytes_per_elem;
    void* buckets = calloc(cap, sizeof(Node*));
    if(buckets == NULL) {
        LOG_ERROR("Failed to allocate HashMap.");
        return 1;
    }
    hm->buckets = (Node**) buckets;
    return 0;
}

size_t hash(size_t cap, char* key) {
    const size_t BASE = 0x811c9dc5;
    const size_t PRIME = 0x01000193;
    size_t initial = BASE;
    while(*key) {
        initial ^= *key++;
        initial *= PRIME;
    }
    return initial & (cap - 1);
}

unsigned int HashMap_insert(HashMap* hm, char* key, void* val) {
    hm->size++;
    Node* bucket = hm->buckets[hash(hm->cap, key)];
    Node* node;
    if(bucket == NULL) {
        node = (Node*) calloc(1, sizeof(Node));
        hm->buckets[hash(hm->cap, key)] = node;
    } else {
        while(bucket->next != NULL) bucket = bucket->next;
        bucket->next = (Node*) calloc(1, sizeof(Node));
        node = bucket->next;
    }
    if(node == NULL) {
        LOG_ERROR("Unable to insert value in HashMap.");
        return 1;
    }
    node->key = (char*) malloc(strlen(key) + 1);
    if(node->key == NULL) {
        LOG_ERROR("Unable to insert value in HashMap.");
        return 1;
    }
    strcpy(node->key, key);
    node->item = malloc(hm->bytes_per_elem);
    if(node->item == NULL) {
        LOG_ERROR("Unable to insert value in HashMap.");
        return 1;
    }
    memcpy(node->item, val, hm->bytes_per_elem);
    return 0;
}

void* HashMap_get(HashMap hm, char* key) {
    Node* node = hm.buckets[hash(hm.cap, key)];
    while(node != NULL) {
        if(strcmp(node->key, key) == 0) return node->item;
        node = node->next;
    }
    return NULL;
}

void HashMap_free(HashMap hm) {
    for(size_t i = 0; i < hm.cap; ++i) {
        Node* bucket = hm.buckets[i];
        if(bucket != NULL) Node_free(*bucket);
    }
    free(hm.buckets);
}

void HashMap_dump(HashMap hm, void (*func)(char*, void*)) {
    for(size_t i = 0; i < hm.cap; ++i) {
        Node* node = hm.buckets[i];
        while(node != NULL) {
            func(node->key, node->item);
            node = node->next;
        }
    }
}

#endif /* HASHMAP_H */
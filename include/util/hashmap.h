#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

typedef struct __Node Node;
struct __Node {
    Node* next;
    char contents[];
};

Node* Node_alloc(char* key, void* val, size_t bytes_per_elem) {
    size_t bytes_per_key = sizeof(char) * (strlen(key) + 1);
    Node* node = (Node*) malloc(sizeof(Node) + bytes_per_key + bytes_per_elem);
    if(node == NULL) return NULL;
    node->next = NULL;
    strcpy(node->contents, key);
    memcpy(&(node->contents[bytes_per_key]), (char*) val, bytes_per_elem);
    return node;
}

void* Node_value(Node* node) {
    return (void*) &(node->contents[strlen(node->contents) + 1]);
}

typedef struct {
    size_t size;
    size_t bytes_per_elem;
    size_t bucket_count;
    Node** buckets;
} HashMap;

unsigned int HashMap_init(HashMap* hm, size_t bucket_count, size_t bytes_per_elem) {
    hm->size = 0;
    hm->bytes_per_elem = bytes_per_elem;
    hm->bucket_count = bucket_count;
    void* buckets = calloc(bucket_count, sizeof(Node*));
    if(buckets == NULL) {
        LOG_ERROR("Failed to allocate HashMap.");
        return 1;
    }
    hm->buckets = (Node**) buckets;
    return 0;
}

size_t hash(size_t bucket_count, char* key) {
    const size_t BASE = 0x811c9dc5;
    const size_t PRIME = 0x01000193;
    size_t initial = BASE;
    while(*key) {
        initial ^= *key++;
        initial *= PRIME;
    }
    return initial & (bucket_count - 1);
}

unsigned int HashMap_insert(HashMap* hm, char* key, void* val) {
    hm->size++;
    Node* bucket = hm->buckets[hash(hm->bucket_count, key)];
    Node* node;
    if(bucket == NULL) {
        node = Node_alloc(key, val, hm->bytes_per_elem);
        hm->buckets[hash(hm->bucket_count, key)] = node;
    } else {
        while(bucket->next != NULL) bucket = bucket->next;
        bucket->next = Node_alloc(key, val, hm->bytes_per_elem);
        node = bucket->next;
    }
    if(node == NULL) {
        LOG_ERROR("Unable to insert value in HashMap.");
        return 1;
    }
    return 0;
}

void* HashMap_get(HashMap hm, char* key) {
    Node* node = hm.buckets[hash(hm.bucket_count, key)];
    while(node != NULL) {
        if(strcmp(node->contents, key) == 0) return Node_value(node);
        node = node->next;
    }
    return NULL;
}

void HashMap_free(HashMap hm) {
    for(size_t i = 0; i < hm.bucket_count; ++i) {
        Node* bucket = hm.buckets[i];
        if(bucket != NULL) free(bucket);
    }
    free(hm.buckets);
}

void HashMap_dump(HashMap hm, void (*func)(char*, void*)) {
    for(size_t i = 0; i < hm.bucket_count; ++i) {
        Node* node = hm.buckets[i];
        while(node != NULL) {
            func(node->contents, Node_value(node));
            node = node->next;
        }
    }
}

#endif /* HASHMAP_H */
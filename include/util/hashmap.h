#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"

typedef struct __HASHMAP_H__Node Node;
struct __HASHMAP_H__Node {
    Node* next;
    char contents[];
};

#pragma GCC diagnostic ignored "-Wunused-function"
static Node* Node_alloc(const char* key, const void* val, size_t bytes_per_elem) {
    size_t bytes_per_key = sizeof(char) * (strlen(key) + 1);
    Node* node = (Node*) malloc(sizeof(Node) + bytes_per_key + bytes_per_elem);
    if(node == NULL) return NULL;
    node->next = NULL;
    strcpy(node->contents, key);
    memcpy(&(node->contents[bytes_per_key]), (char*) val, bytes_per_elem);
    return node;
}

#pragma GCC diagnostic ignored "-Wunused-function"
static void* Node_value(Node* node) {
    return (void*) &(node->contents[strlen(node->contents) + 1]);
}

typedef struct {
    size_t size;
    size_t bytes_per_elem;
    size_t bucket_count;
    Node** buckets;
} HashMap;

#pragma GCC diagnostic ignored "-Wunused-function"
static unsigned int HashMap_init(HashMap* hm, size_t bucket_count, size_t bytes_per_elem) {
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

#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-conversion"
static size_t hash(size_t bucket_count, const char* const key) {
    const char* key_inner = key;
    const size_t BASE = 0x811c9dc5;
    const size_t PRIME = 0x01000193;
    size_t initial = BASE;
    while(*key_inner) {
        initial ^= *key_inner++;
        initial *= PRIME;
    }
    return initial & (bucket_count - 1);
}

#pragma GCC diagnostic ignored "-Wunused-function"
static unsigned int HashMap_insert(HashMap* hm, const char* const key, const void* const val) {
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
        free(node);
        return 1;
    }
    return 0;
}

#pragma GCC diagnostic ignored "-Wunused-function"
static void* HashMap_get(HashMap hm, const char* const key) {
    Node* node = hm.buckets[hash(hm.bucket_count, key)];
    while(node != NULL) {
        if(strcmp(node->contents, key) == 0) return Node_value(node);
        node = node->next;
    }
    return NULL;
}

#pragma GCC diagnostic ignored "-Wunused-function"
static void HashMap_free(HashMap hm) {
    Node* bucket;
    Node* temp;
    for(size_t i = 0; i < hm.bucket_count; ++i) {
        bucket = hm.buckets[i];
        while(bucket != NULL) {
            temp = bucket->next;
            free(bucket);
            bucket = temp;
        }
    }
    free(hm.buckets);
}

#pragma GCC diagnostic ignored "-Wunused-function"
static void HashMap_dump(HashMap hm, void (*func)(char*, void*)) {
    Node* node;
    for(size_t i = 0; i < hm.bucket_count; ++i) {
        node = hm.buckets[i];
        while(node != NULL) {
            func(node->contents, Node_value(node));
            node = node->next;
        }
    }
}

#endif /* __HASHMAP_H__ */

#ifndef HASHTABLE_H
#define HASHTABLE_H

typedef unsigned long (*hash_func_t)(const void *key);
typedef int (*key_equals_t)(const void *key1, const void *key2);
typedef void *(*dup_func_t)(const void *data);
typedef void (*free_func_t)(void *data);

typedef struct hash_entry
{
    void *key;
    void *value;
    struct hash_entry *next;
} hash_entry_t;

/* Generic hash table structure */
typedef struct hashtable {
    size_t size;             /* Number of buckets */
    hash_entry_t **buckets;  /* Array of bucket pointers */
    /* Function pointers for key/value operations */
    hash_func_t hash_func;
    key_equals_t key_equals;
    dup_func_t key_dup;
    dup_func_t value_dup;
    free_func_t key_free;
    free_func_t value_free;
} hashtable_t;

#endif

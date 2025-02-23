#include "hashtable.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

hashtable_t* hashtable_create(size_t size, hash_func_t hash_func, key_equals_t key_equals,
                                dup_func_t key_dup, dup_func_t value_dup,
                                free_func_t key_free, free_func_t value_free)
{
    hashtable_t *table = malloc(sizeof(hashtable_t));
    if (!table) return NULL;
    table->size = size;
    table->buckets = calloc(size, sizeof(hash_entry_t *)); // doesn't exists yet
    if (!table->buckets)
    {
        free(table);
        return NULL;
    }
    table->hash_func = hash_func;
    table->key_equals = key_equals;
    table->key_dup = key_dup;
    table->value_dup = value_dup;
    table->key_free = key_free;
    table->value_free = value_free;
    return table;
}

void hashtable_destroy(hashtable_t *table)
{
    if (!table) return;
    for (size_t i = 0; i < table->size; i++)
    {
        hash_entry_t *entry = table->buckets[i];
        while (entry)
        {
            hash_entry_t *next = entry->next;
            if (table->key_free)
                table->key_free(entry->key);
            if (table->value_free)
                table->value_free(entry->value);
            free(entry);
            entry = next;
        }
    }
    free(table->buckets);
    free(table);
}

int hashtable_set(hashtable_t *table, const void *key, const void *value)
{
    if (!table || !key || !value) return -1;
    unsigned long hash = table->hash_func(key);
    size_t index = hash % table->size;

    hash_entry_t *entry = table->buckets[index];
    while (entry)
    {
        if (table->key_equals(entry->key, key) == 0)
        {
            /* Key exists: update its value */
            void *new_value = table->value_dup ? table->value_dup(value) : (void *)value;
            if (!new_value) return -1;
            if (table->value_free)
                table->value_free(entry->value);
            entry->value = new_value;
            return 0;
        }
        entry = entry->next;
    }

    hash_entry_t *new_entry = malloc(sizeof(hash_entry_t));
    if (!new_entry) return -1;
    new_entry->key = table->key_dup ? table->key_dup(key) : (void *)key;
    new_entry->value = table->value_dup ? table->value_dup(value) : (void *)value;
    if (!new_entry->key || !new_entry->value)
    {
        if (table->key_free) table->key_free(new_entry->key);
        if (table->value_free) table->value_free(new_entry->value);
        free(new_entry);
        return -1;
    }
    new_entry->next = table->buckets[index];
    table->buckets[index] = new_entry;
    return 0;
}

void* hashtable_get(hashtable_t *table, const void *key)
{
    if (!table || !key) return NULL;
    unsigned long hash = table->hash_func(key);
    size_t index = hash % table->size;
    hash_entry_t *entry = table->buckets[index];
    while (entry)
    {
        if (table->key_equals(entry->key, key) == 0)
            return entry->value;
        entry = entry->next;
    }
    return NULL;
}

int hashtable_remove(hashtable_t *table, const void *key)
{
    if (!table || !key) return -1;
    unsigned long hash = table->hash_func(key);
    size_t index = hash % table->size;
    hash_entry_t *entry = table->buckets[index];
    hash_entry_t *prev = NULL;
    while (entry)
    {
        if (table->key_equals(entry->key, key) == 0)
        {
            if (prev)
                prev->next = entry->next;
            else
                table->buckets[index] = entry->next;
            if (table->key_free) table->key_free(entry->key);
            if (table->value_free) table->value_free(entry->value);
            free(entry);
            return 0;
        }
        prev = entry;
        entry = entry->next;
    }
    return -1;
}

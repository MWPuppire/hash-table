#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct {
	size_t capacity;
	size_t size;
	struct _ht_item *items;
} ht_hash_table;

ht_hash_table *ht_new(void);
void ht_destroy(ht_hash_table *ht);
bool ht_contains(ht_hash_table *ht, const char *key);
void ht_resize(ht_hash_table *ht, size_t min_size);
void ht_insert(ht_hash_table *ht, const char *key, const char *value);
void ht_insert_unique(ht_hash_table *ht, const char *key, const char *value);
char *ht_search(ht_hash_table *ht, const char *key);
void ht_remove(ht_hash_table *ht, const char *key);

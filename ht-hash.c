#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "ht-hash.h"

struct _ht_item {
	char *key;
	char *value;
};

ht_hash_table *ht_new(void) {
	ht_hash_table *ht = malloc(sizeof(ht_hash_table));
	ht->capacity = 64;
	ht->size = 0;
	ht->items = calloc(64, sizeof(struct _ht_item));
	return ht;
}

void ht_destroy(ht_hash_table *ht) {
	for (size_t i = 0; i < ht->capacity; i++) {
		if (ht->items[i].key != NULL) {
			free(ht->items[i].key);
			free(ht->items[i].value);
		}
	}
	free(ht);
}

static const size_t FIB_MULT = 11400714819323198485ull;
static const size_t HT_PRIME = 151;

static size_t ht_hash(const char *s, size_t cap) {
	int shift = 64 - __builtin_ctz(cap);
	size_t hash = 0;
	size_t len = strlen(s);
	for (size_t i = 0; i < len; i++) {
		hash += (long) pow(HT_PRIME, len - (i + 1)) * s[i];
	}
	return (hash * FIB_MULT) >> shift;
}

static bool ht_find(ht_hash_table *ht, const char *key, size_t *out_index) {
	size_t cap = ht->capacity;
	size_t index = ht_hash(key, cap);
	if (ht->items[index].key != NULL) {
		size_t attempts = 0;
		while (ht->items[index].key != NULL) {
			if (strcmp(ht->items[index].key, key) == 0) {
				*out_index = index;
				return true;
			}
			index = (index + (attempts << 1) + 1) & (cap - 1);
			if (attempts++ > ht->size << 1) {
				return false;
			}
		}
	}
	return false;
}

static void ht_insert_inner(struct _ht_item *items, size_t cap, char *key, char *value) {
	size_t index = ht_hash(key, cap);
	if (items[index].key != NULL) {
		size_t attempts = 0;
		while (items[index].key != NULL) {
			index = (index + (attempts++ << 1) + 1) & (cap - 1);
		}
	}
	items[index].key = key;
	items[index].value = value;
}

bool ht_contains(ht_hash_table *ht, const char *key) {
	size_t _drop;
	return ht_find(ht, key, &_drop);
}

void ht_resize(ht_hash_table *ht, size_t min_size) {
	size_t old_cap = ht->capacity;
	// round up to next power of 2
	size_t new_cap = min_size;
	new_cap--;
	new_cap |= new_cap >> 1;
	new_cap |= new_cap >> 2;
	new_cap |= new_cap >> 4;
	new_cap |= new_cap >> 8;
	new_cap |= new_cap >> 16;
	new_cap |= new_cap >> 32;
	new_cap++;

	if (new_cap <= old_cap) {
		return;
	}
	struct _ht_item *old_items = ht->items;
	struct _ht_item *new_items = calloc(new_cap, sizeof(struct _ht_item));
	for (size_t i = 0; i < old_cap; i++) {
		if (old_items[i].key != NULL) {
			ht_insert_inner(new_items, new_cap, old_items[i].key, old_items[i].value);
		}
	}
	ht->items = new_items;
	ht->capacity = new_cap;
	free(old_items);
}

void ht_insert(ht_hash_table *ht, const char *key, const char *value) {
	size_t cur_index;
	bool contains = ht_find(ht, key, &cur_index);
	if (contains) {
		free(ht->items[cur_index].value);
		ht->items[cur_index].value = strdup(value);
		return;
	}
	size_t cap = ht->capacity;
	// resize on 75% capacity
	if (ht->size++ << 2 > (cap << 1) + cap) {
		ht_resize(ht, cap << 1);
		return ht_insert_inner(ht->items, cap << 1, strdup(key), strdup(value));
	} else {
		return ht_insert_inner(ht->items, cap, strdup(key), strdup(value));
	}
}

void ht_insert_unique(ht_hash_table *ht, const char *key, const char *value) {
	size_t cap = ht->capacity;
	// resize on 75% capacity
	if (ht->size++ << 2 > (cap << 1) + cap) {
		ht_resize(ht, cap << 1);
		return ht_insert_inner(ht->items, cap << 1, strdup(key), strdup(value));
	} else {
		return ht_insert_inner(ht->items, cap, strdup(key), strdup(value));
	}
}

char *ht_search(ht_hash_table *ht, const char *key) {
	size_t index;
	if (ht_find(ht, key, &index)) {
		return ht->items[index].value;
	} else {
		return NULL;
	}
}

void ht_remove(ht_hash_table *ht, const char *key) {
	size_t index;
	if (ht_find(ht, key, &index)) {
		free(ht->items[index].key);
		ht->size--;
		ht->items[index].key = NULL;
	}
}
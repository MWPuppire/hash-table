#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ht-hash.h"

const size_t HT_INITIAL_CAPACITY = 64;

/** \internal */
struct _ht_item {
	char *key;
	char *value;
};

ht_hash_table *ht_new(void) {
	ht_hash_table *ht = malloc(sizeof(ht_hash_table));
	if (__builtin_expect(ht == NULL, 0)) {
		return NULL;
	}
	ht->capacity = 0;
	ht->size = 0;
	ht->items = NULL;
	return ht;
}

void ht_destroy(ht_hash_table *ht) {
	for (size_t i = 0; i < ht->capacity; i++) {
		if (ht->items[i].key != NULL) {
			free(ht->items[i].key);
			free(ht->items[i].value);
		}
	}
	free(ht->items);
	free(ht);
}

void ht_clear(ht_hash_table *ht) {
	for (size_t i = 0; i < ht->capacity; i++) {
		if (ht->items[i].key != NULL) {
			free(ht->items[i].key);
			free(ht->items[i].value);
			ht->items[i].key = NULL;
		}
	}
	free(ht->items);
	memset(ht, 0, sizeof(ht_hash_table));
}

static const size_t FIB_MULT = 11400714819323198485ull;
static const size_t HT_PRIME = 151;

__attribute__((nonnull(1), pure, nothrow))
static size_t ht_hash(const char *s, size_t len, size_t cap) {
	if (cap == 0) {
		return 0;
	}
	int shift = 64 - __builtin_ctz(cap);
	size_t hash = 0;
	size_t mult = 1;
	for (size_t i = 0; i < len; i++) {
		hash += mult * s[i];
		mult *= HT_PRIME;
	}
	return (hash * FIB_MULT) >> shift;
}

/// Generalizes `search` and `contains`; returns `true` if `key` is in `ht`, and
/// assigns the index of the key in `items` to `out_index` upon finding.
/// `out_index` is assumed to be non-`NULL`, since the function is private;
/// calling it with `NULL` for `out_index` will seg-fault.
__attribute__((nonnull(1, 2, 3), pure, nothrow))
static bool ht_find(const ht_hash_table *ht, const char *key, size_t *out_index) {
	size_t cap = ht->capacity;
	if (cap == 0) {
		return false;
	}
	size_t len = strlen(key);
	size_t index = ht_hash(key, len, cap);
	size_t attempts = 0;
	while (ht->items[index].key != NULL) {
		if (strncmp(ht->items[index].key, key, len) == 0) {
			*out_index = index;
			return true;
		}
		index = (index + (attempts << 1) + 1) & (cap - 1);
		if (attempts++ > ht->size << 1) {
			return false;
		}
	}
	return false;
}

/// Insert without checking for existing keys or testing capacity.
__attribute__((nonnull(1, 3, 4), nothrow))
static bool ht_insert_inner(struct _ht_item *items, size_t cap, char *key, char *value) {
	assert(cap != 0);
	size_t index = ht_hash(key, strlen(key), cap);
	size_t attempts = 0;
	while (items[index].key != NULL) {
		index = (index + (attempts++ << 1) + 1) & (cap - 1);
	}
	items[index].key = key;
	items[index].value = value;
	return true;
}

bool ht_contains(const ht_hash_table *ht, const char *key) {
	size_t _drop;
	return ht_find(ht, key, &_drop);
}

__attribute__((nonnull(1), nothrow))
static void ht_resize_exact(ht_hash_table *ht, size_t old_cap, size_t new_cap) {
	struct _ht_item *old_items = ht->items;
	struct _ht_item *new_items = calloc(new_cap, sizeof(struct _ht_item));
	if (__builtin_expect(new_items == NULL, 0)) {
		return;
	}
	for (size_t i = 0; i < old_cap; i++) {
		if (old_items[i].key != NULL) {
			ht_insert_inner(new_items, new_cap, old_items[i].key, old_items[i].value);
		}
	}
	ht->items = new_items;
	ht->capacity = new_cap;
	free(old_items);
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

	// they're probably only calling `resize` with a larger size
	if (__builtin_expect(new_cap <= old_cap, 0)) {
		return;
	}
	ht_resize_exact(ht, old_cap, new_cap);
}

void ht_shrink_to_fit(ht_hash_table *ht) {
	size_t old_cap = ht->capacity;
	// round up to next power of 2
	size_t new_cap = ht->size;
	if (ht->size == 0) {
		return ht_clear(ht);
	}
	new_cap--;
	new_cap |= new_cap >> 1;
	new_cap |= new_cap >> 2;
	new_cap |= new_cap >> 4;
	new_cap |= new_cap >> 8;
	new_cap |= new_cap >> 16;
	new_cap |= new_cap >> 32;
	new_cap++;

	if (new_cap == old_cap) {
		return;
	}
	ht_resize_exact(ht, old_cap, new_cap);
}

bool ht_insert(ht_hash_table *ht, const char *key, const char *value) {
	size_t cur_index;
	bool contains = ht_find(ht, key, &cur_index);
	if (contains) {
		char *val_clone;
		if (__builtin_expect((val_clone = strdup(value)) == NULL, 0)) {
			return false;
		}
		free(ht->items[cur_index].value);
		ht->items[cur_index].value = val_clone;
		return true;
	}
	return ht_insert_unique(ht, key, value);
}

bool ht_insert_unique(ht_hash_table *ht, const char *key, const char *value) {
	char *key_clone, *val_clone;
	if (__builtin_expect((key_clone = strdup(key)) == NULL, 0)) {
		return false;
	} else if (__builtin_expect((val_clone = strdup(value)) == NULL, 0)) {
		return false;
	}
	size_t cap = ht->capacity;
	// in case there haven't been any items added yet
	if (__builtin_expect(cap == 0, 0)) {
		ht->items = calloc(HT_INITIAL_CAPACITY, sizeof(struct _ht_item));
		if (__builtin_expect(ht->items == NULL, 0)) {
			return false;
		}
		cap = ht->capacity = HT_INITIAL_CAPACITY;
		// no need to use `ht_insert_inner` for the first item
		size_t idx = ht_hash(key_clone, strlen(key_clone), cap);
		ht->items[idx].key = key_clone;
		ht->items[idx].value = val_clone;
		return true;
	}
	// resize on 75% capacity; expect it to have enough size, normally
	if (__builtin_expect(ht->size++ << 2 > (cap << 1) + cap, 0)) {
		ht_resize_exact(ht, cap, cap << 1);
		return ht_insert_inner(ht->items, cap << 1, key_clone, val_clone);
	} else {
		return ht_insert_inner(ht->items, cap, key_clone, val_clone);
	}
}

char *ht_search(const ht_hash_table *ht, const char *key) {
	size_t index;
	// keys being searched for probably exist
	if (__builtin_expect(ht_find(ht, key, &index), 1)) {
		return ht->items[index].value;
	} else {
		return NULL;
	}
}

bool ht_remove(ht_hash_table *ht, const char *key) {
	size_t index;
	// keys being removed probably exist
	if (__builtin_expect(ht_find(ht, key, &index), 1)) {
		free(ht->items[index].key);
		free(ht->items[index].value);
		ht->size--;
		ht->items[index].key = NULL;
		return true;
	}
	return false;
}

char *ht_remove_get(ht_hash_table *ht, const char *key) {
	size_t index;
	// keys being removed probably exist
	if (__builtin_expect(ht_find(ht, key, &index), 1)) {
		free(ht->items[index].key);
		ht->size--;
		ht->items[index].key = NULL;
		return ht->items[index].value;
	}
	return NULL;
}

ht_iter ht_iterator(const ht_hash_table *ht) {
	return (ht_iter) {
		.next = ht->items,
		.end = ht->items + ht->capacity,
	};
}

char *ht_iter_next(ht_iter *iter) {
	// a caller will probably stop advancing an iterator after it finishes,
	// so most calls will probably continue to yield values
	while (__builtin_expect(iter->next != iter->end, 1)) {
		struct _ht_item *item = iter->next++;
		if (item->key != NULL) {
			return item->key;
		}
	}
	return NULL;
}

bool ht_iter_next_pair(ht_iter *iter, char **key, char **val) {
	// a caller will probably stop advancing an iterator after it finishes,
	// so most calls will probably continue to yield values
	while (__builtin_expect(iter->next != iter->end, 1)) {
		struct _ht_item *item = iter->next++;
		if (item->key != NULL) {
			key != NULL && (*key = item->key);
			val != NULL && (*val = item->value);
			return true;
		}
	}
	return false;
}

/// Much simpler version of `ht_json_stringify` in the event `out` is `NULL`, to
/// avoid unnecessary allocation and buffer-writing.
__attribute__((nonnull(1), nothrow))
static size_t ht_json_dry_run(const ht_hash_table *ht) {
	size_t len = 1;
	ht_iter iter = ht_iterator(ht);
	char *key, *val;
	while (ht_iter_next_pair(&iter, &key, &val)) {
		len++; // quote
		len += strlen(key);
		len += 3; // close quote, colon, open quote
		len += strlen(val);
		len += 2; // close quote, comma
	}
	// closing comma becomes closing bracket for length
	return len + __builtin_expect(len == 1, 0);
}

size_t ht_json_stringify(const ht_hash_table *ht, char **out) {
	if (out == NULL) {
		return ht_json_dry_run(ht);
	}
	// Probably a reasonable starting point, factoring in quotes, commas,
	// colons, and assuming keys/values typically are less than 10 letters.
	// Add 5 in case there are no items in `ht`, so it can still hold `{}`.
	size_t cap = ht->size * 20 + 5;
	// test no overflow happened
	assert(cap > ht->size);
	char *buf = malloc(cap);
	*buf = '{';
	size_t pos = 1;

	ht_iter iter = ht_iterator(ht);
	char *key, *val;
	while (ht_iter_next_pair(&iter, &key, &val)) {
		size_t key_len = strlen(key), val_len = strlen(val);
		while (pos + key_len + val_len + 6 > cap) {
			// test no overflow
			assert(cap * 2 > cap);
			cap *= 2;
			char *new_buf = realloc(buf, cap);
			if (__builtin_expect(new_buf == NULL, 0)) {
				free(buf);
				return 0;
			}
			buf = new_buf;
		}
		buf[pos++] = '"';
		memcpy(buf + pos, key, key_len);
		pos += key_len;
		buf[pos++] = '"';
		buf[pos++] = ':';
		buf[pos++] = '"';
		memcpy(buf + pos, val, val_len);
		pos += val_len;
		buf[pos++] = '"';
		buf[pos++] = ',';
	}
	// overwrite the comma, since JSON can't have trailing commas
	if (__builtin_expect(pos == 1, 0)) {
		// add one in case there were no commas, though
		pos++;
	}
	buf[pos - 1] = '}';
	buf[pos] = '\0';
	*out = buf;
	return pos;
}

/// Much simpler version of `ht_json_stringify_escape` in the event `out` is
/// `NULL`, to avoid unnecessary allocation and buffer-writing.
__attribute__((nonnull(1), nothrow))
static size_t ht_json_dry_run_escape(const ht_hash_table *ht) {
	size_t len = 1;
	ht_iter iter = ht_iterator(ht);
	char *key, *val;
	while (ht_iter_next_pair(&iter, &key, &val)) {
		len++; // quote
		size_t key_len = strlen(key);
		len += key_len;
		for (size_t i = 0; i < key_len; i++) {
			len += key[i] == '"';
		}
		len += 3; // close quote, colon, open quote
		size_t val_len = strlen(val);
		len += val_len;
		for (size_t i = 0; i < val_len; i++) {
			len += val[i] == '"';
		}
		len += 2; // close quote, comma
	}
	// closing comma becomes closing bracket for length
	return len + __builtin_expect(len == 1, 0);
}

size_t ht_json_stringify_escape(const ht_hash_table *ht, char **out) {
	if (out == NULL) {
		return ht_json_dry_run_escape(ht);
	}
	// Probably a reasonable starting point, factoring in quotes, commas,
	// colons, and assuming keys/values typically are less than 10 letters.
	// Add 5 in case there are no items in `ht`, so it can still hold `{}`.
	size_t cap = ht->size * 24 + 5;
	// test no overflow happened
	assert(cap > ht->size);
	char *buf = malloc(cap);
	*buf = '{';
	size_t pos = 1;

	ht_iter iter = ht_iterator(ht);
	char *key, *val;
	while (ht_iter_next_pair(&iter, &key, &val)) {
		char *base_key = key, *base_val = val;
		size_t key_len = strlen(key), val_len = strlen(val);
		// Double length of key/value length for determining capacity,
		// in the event the key or value consists only of quotes.
		while (pos + key_len * 2 + val_len * 2 + 6 > cap) {
			assert(cap * 2 > cap);
			cap *= 2;
			char *new_buf = realloc(buf, cap);
			if (__builtin_expect(new_buf == NULL, 0)) {
				free(buf);
				return 0;
			}
			buf = new_buf;
		}
		buf[pos++] = '"';
		while ((key = strchr(key, '"')) != NULL) {
			size_t diff = key - base_key;
			memcpy(buf + pos, base_key, diff);
			pos += diff;
			key_len -= diff;
			key_len--;
			base_key = ++key;
			buf[pos++] = '\\';
			buf[pos++] = '"';
		}
		memcpy(buf + pos, base_key, key_len);
		pos += key_len;
		buf[pos++] = '"';
		buf[pos++] = ':';
		buf[pos++] = '"';
		while ((val = strchr(val, '"')) != NULL) {
			size_t diff = val - base_val;
			memcpy(buf + pos, base_val, diff);
			pos += diff;
			val_len -= diff;
			val_len--;
			base_val = ++val;
			buf[pos++] = '\\';
			buf[pos++] = '"';
		}
		memcpy(buf + pos, base_val, val_len);
		pos += val_len;
		buf[pos++] = '"';
		buf[pos++] = ',';
	}
	// overwrite the comma, since JSON can't have trailing commas
	if (__builtin_expect(pos == 1, 0)) {
		// add one in case there were no commas, though
		pos++;
	}
	buf[pos - 1] = '}';
	buf[pos] = '\0';
	*out = buf;
	return pos;
}

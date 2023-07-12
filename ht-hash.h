#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct {
	size_t capacity;
	size_t size;
	struct _ht_item *items;
} ht_hash_table;

typedef struct {
	struct _ht_item *next;
	struct _ht_item *end;
} ht_iter;

/// Creates a new empty hash table. The caller is responsible for calling
/// `ht_destroy` to avoid memory leaks.
ht_hash_table *ht_new(void);
/// Releases memory associated with the hash table. This includes all items
/// contained within; pointers to keys and values from the hash table are
/// invalidated after calling this.
void ht_destroy(ht_hash_table *ht);
/// Returns `true` if `key` exists in `ht`, and `false` otherwise.
bool ht_contains(const ht_hash_table *ht, const char *key);
/// Resizes `ht` to ensure it has enough capacity to hold at least `min_size`
/// items. `resize` never shrinks `ht`.
void ht_resize(ht_hash_table *ht, size_t min_size);
/// Shrinks `ht` to the smallest size that can hold all its items. Note that, as
/// currently implemented, capacity has to be a power of two, so capacity may
/// still end up significantly larger than size.
void ht_shrink_to_fit(ht_hash_table *ht);
/// Sets `key` to equal `value` in the hash table, replacing any existing value
/// associated with `key` with the new value.
void ht_insert(ht_hash_table *ht, const char *key, const char *value);
/// Insert a key which is known to not be contained in `ht`. This can be faster
/// than a normal insertion, since it can avoid testing equality with keys, but
/// behavior is undefined if `key` does exist within `ht` before calling.
void ht_insert_unique(ht_hash_table *ht, const char *key, const char *value);
/// Returns the value found at `key` in `ht`, or `NULL` if the key doesn't exist
/// in `ht`.
char *ht_search(const ht_hash_table *ht, const char *key);
/// Removes `key` from `ht` and frees the value associated with it.
void ht_remove(ht_hash_table *ht, const char *key);
/// Removes `key` from `ht` and returns the value that was associated with it.
/// The caller is responsible for freeing the returned value.
char *ht_remove_get(ht_hash_table *ht, const char *key);
/// Returns an iterator over the hash table. The iterator can be advanced with
/// either `ht_iter_next` or `ht_iter_next_pair`.
/// Note that using any operations which may reallocate the buffer will
/// invalidate the iterator (this includes `resize`, `shrink_to_fit`, and
/// `insert` operations). `const` operations are still safe to use, as well as
/// `remove` in current implementation.
ht_iter ht_iterator(const ht_hash_table *ht);
/// Advances `iter` and returns the next key, or `NULL` if the iterator was
/// empty.
char *ht_iter_next(ht_iter *iter);
/// Advances `iter` and returns `true` if there was another item, or `false` if
/// the iterator was empty. If passed a non-`NULL` value, `key` and `val` will
/// be set to the key and value of the next item, respectively.
bool ht_iter_next_pair(ht_iter *iter, char **key, char **val);
/// Creates a JSON string of `table` and places it in `out`, overwriting any
/// existing value. Returns the length of the string, in bytes. The caller is
/// responsible for freeing `out`.
/// Note that this assumes keys and values are valid JSON strings (e.g. don't
/// contain unescaped quotes or invalid escape sequences), and it does nothing
/// to escape strings.
/// On a failure, the length returned is 0, and `out` is not changed (nor is any
/// memory allocated). A valid JSON string, due to opening & closing braces,
/// will never be 0 bytes.
size_t ht_json_stringify(const ht_hash_table *table, char **out);

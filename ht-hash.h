/**
 * \file ht-hash.h
 * \brief A hash table implementation.
 *
 * A simple hash table in C. Only supports string keys and string values.
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>

/** \internal */
struct _ht_item;

/**
 * \struct ht_hash_table
 * \brief The hash table class.
 *
 * A hash table for string keys and values. Should be zero-initialized.
 * `_ht_item` is intentionally kept opaque; access to stored values should only
 * be through `ht` functions.
 * `ht_clear` should be called before a table exits scope if any values were
 * inserted into it to properly release memory.
 */
typedef struct {
	size_t capacity;
	size_t size;
	struct _ht_item *items;
} ht_hash_table;

/**
 * \struct ht_iter
 * \brief A hash table iterator.
 *
 * An iterator over a hash table. Created through the `ht_iterator` function.
 */
typedef struct {
	struct _ht_item *next;
	struct _ht_item *end;
} ht_iter;

#ifndef MAKE_DOCS
__attribute__((nonnull(1), nothrow))
#endif
/**
 * \brief Removes all items from `ht`.
 *
 * Removes all items from `ht`, freeing the memory associated with them and
 * resetting `size` and `capacity` to 0. Must be called to avoid memory leaks
 * before letting a hash table leave scope, if any values were added to the
 * table.
 *
 * \memberof ht_hash_table
 * \param ht The hash table to clear
 */
void ht_clear(ht_hash_table *ht);

#ifndef MAKE_DOCS
__attribute__((nonnull(1, 2), nothrow, pure))
#endif
/**
 * \brief Tests if `ht` contains `key`, with specified key length.
 *
 * Returns `true` if `key` exists in `ht`, and `false` otherwise.
 *
 * \memberof ht_hash_table
 * \param ht The table to search in
 * \param key The key to search for
 * \param key_len The length of `key`, in bytes
 */
bool ht_containsn(const ht_hash_table *ht, const char *key, size_t key_len);

#ifndef MAKE_DOCS
__attribute__((nonnull(1, 2), nothrow, pure))
#endif
/**
 * \brief Tests if `ht` contains `key`.
 *
 * Returns `true` if `key` exists in `ht`, and `false` otherwise. Key length is
 * determined via the `strlen` function.
 *
 * \memberof ht_hash_table
 * \param ht The table to search in
 * \param key The key to search for
 */
bool ht_contains(const ht_hash_table *ht, const char *key);

#ifndef MAKE_DOCS
__attribute__((nonnull(1), nothrow))
#endif
/**
 * \brief Resizes `ht` to hold at least `min_size` items.
 *
 * Resizes `ht` to ensure it has enough capacity to hold at least `min_size`
 * items. `resize` never lowers capacity of a hash-table, though; if you want to
 * reduce memory usage, use `ht_shrink_to_fit` instead.
 *
 * \memberof ht_hash_table
 * \param ht The table to resize
 * \param min_size The minimum number of items `ht` should be able to hold
 */
void ht_resize(ht_hash_table *ht, size_t min_size);

#ifndef MAKE_DOCS
__attribute__((nonnull(1), nothrow))
#endif
/**
 * \brief Shrinks `ht` to the smallest size it can be.
 *
 * Reduces the capacity of `ht` to the smallest that can hold all of its items.
 * Note that, as currently implemented, capacity has to be a power of two, so
 * capacity may still end up significantly larger than the number of items.
 *
 * \memberof ht_hash_table
 * \param ht The table to resize
 */
void ht_shrink_to_fit(ht_hash_table *ht);

#ifndef MAKE_DOCS
__attribute__((nonnull(1, 2, 4), nothrow))
#endif
/**
 * \brief Inserts `key` holding `value` into `ht`, with specified key and value
 * lengths.
 *
 * Sets `key` to equal `value` in the hash table, replacing any existing value
 * associated with `key` with the new value. Returns `true` if `key` was added
 * successfully, and `false` on error.
 *
 * \memberof ht_hash_table
 * \param ht The table to insert into
 * \param key The key to insert under
 * \param key_len The length of `key`, in bytes
 * \param value The value to insert under `key`
 * \param val_len The length of `value`, in bytes
 */
bool ht_insertn(ht_hash_table *ht, const char *key, size_t key_len, const char *value, size_t val_len);

#ifndef MAKE_DOCS
__attribute__((nonnull(1, 2, 4), nothrow))
#endif
/**
 * \brief Inserts `key` holding `value` into `ht`, where `key` didn't exist
 * previously in `ht`, with specified key and value lengths.
 *
 * Inserts a key which is known not to exist in `ht`, as `ht_insert`. This can
 * be faster than a normal insertion, since it can avoid testing equality with
 * keys, but behavior is undefined if `key` is contained within `ht` before
 * calling. Returns `true` on success and `false` on error.
 *
 * \memberof ht_hash_table
 * \param ht The table to insert into
 * \param key The key to insert under
 * \param key_len The length of `key`, in bytes
 * \param value The value to insert under `key`
 * \param val_len The length of `value`, in bytes
 */
bool ht_insertn_unique(ht_hash_table *ht, const char *key, size_t key_len, const char *value, size_t val_len);

#ifndef MAKE_DOCS
__attribute__((nonnull(1, 2, 3), nothrow))
#endif
/**
 * \brief Inserts `key` holding `value` into `ht`.
 *
 * Sets `key` to equal `value` in the hash table, replacing any existing value
 * associated with `key` with the new value. Returns `true` if `key` was added
 * successfully, and `false` on error. Key and value length are determined via
 * the `strlen` function.
 *
 * \memberof ht_hash_table
 * \param ht The table to insert into
 * \param key The key to insert under
 * \param value The value to insert under `key`
 */
bool ht_insert(ht_hash_table *ht, const char *key, const char *value);

#ifndef MAKE_DOCS
__attribute__((nonnull(1, 2, 3), nothrow))
#endif
/**
 * \brief Inserts `key` holding `value` into `ht`, where `key` didn't exist
 * previously in `ht`.
 *
 * Inserts a key which is known not to exist in `ht`, as `ht_insert`. This can
 * be faster than a normal insertion, since it can avoid testing equality with
 * keys, but behavior is undefined if `key` is contained within `ht` before
 * calling. Returns `true` on success and `false` on error. Key and value length
 * are determined via the `strlen` function.
 *
 * \memberof ht_hash_table
 * \param ht The table to insert into
 * \param key The key to insert under
 * \param value The value to insert under `key`
 */
bool ht_insert_unique(ht_hash_table *ht, const char *key, const char *value);

#ifndef MAKE_DOCS
__attribute__((nonnull(1, 2), nothrow, pure))
#endif
/**
 * \brief Returns the value associated with `key`, with specified key length.
 *
 * Looks up `key` in `ht` and returns the associated value, or `NULL` if the key
 * doesn't exist in `ht`.
 *
 * \memberof ht_hash_table
 * \param ht The table to search through
 * \param key The key to search for
 * \param key_len The length of `key`, in bytes
 */
char *ht_searchn(const ht_hash_table *ht, const char *key, size_t key_len);

#ifndef MAKE_DOCS
__attribute__((nonnull(1, 2), nothrow, pure))
#endif
/**
 * \brief Returns the value associated with `key`.
 *
 * Looks up `key` in `ht` and returns the associated value, or `NULL` if the key
 * doesn't exist in `ht`. Key length is determined via the `strlen` function.
 *
 * \memberof ht_hash_table
 * \param ht The table to search through
 * \param key The key to search for
 */
char *ht_search(const ht_hash_table *ht, const char *key);

#ifndef MAKE_DOCS
__attribute__((nonnull(1, 2), nothrow))
#endif
/**
 * \brief Removes `key` from `ht`, with specified key length.
 *
 * Removes `key` from `ht` and frees the value associated with it. Existing
 * references to that value (from, e.g., `ht_search`) are invalidated. Returns
 * `true` if `key` was removed from `ht` and `false` if `key` didn't exist in
 * `ht`. `ht` is unmodified if the key didn't exist in it.
 *
 * \memberof ht_hash_table
 * \param ht The table to remove from
 * \param key The key to remove
 * \param key_len The length of `key`, in bytes
 */
bool ht_removen(ht_hash_table *ht, const char *key, size_t key_len);

#ifndef MAKE_DOCS
__attribute__((nonnull(1, 2), nothrow, warn_unused_result))
#endif
/**
 * \brief Removes `key` from `ht` and returns the value, with specified key
 * length.
 *
 * Removes `key` from `ht` and returns the value that was associated with it.
 * The caller is made responsible for freeing the returned value. This returns
 * `NULL` and has no effect on `ht` if `key` doesn't exist in `ht`.
 *
 * \memberof ht_hash_table
 * \param ht The table to remove from
 * \param key The key to remove
 * \param key_len The length of `key`, in bytes
 */
char *ht_removen_get(ht_hash_table *ht, const char *key, size_t key_len);

#ifndef MAKE_DOCS
__attribute__((nonnull(1, 2), nothrow))
#endif
/**
 * \brief Removes `key` from `ht`.
 *
 * Removes `key` from `ht` and frees the value associated with it. Existing
 * references to that value (from, e.g., `ht_search`) are invalidated. Returns
 * `true` if `key` was removed from `ht` and `false` if `key` didn't exist in
 * `ht`. `ht` is unmodified if the key didn't exist in it. Key length is
 * determined via the `strlen` function.
 *
 * \memberof ht_hash_table
 * \param ht The table to remove from
 * \param key The key to remove
 */
bool ht_remove(ht_hash_table *ht, const char *key);

#ifndef MAKE_DOCS
__attribute__((nonnull(1, 2), nothrow, warn_unused_result))
#endif
/**
 * \brief Removes `key` from `ht` and returns the value.
 *
 * Removes `key` from `ht` and returns the value that was associated with it.
 * The caller is made responsible for freeing the returned value. This returns
 * `NULL` and has no effect on `ht` if `key` doesn't exist in `ht`. Key length
 * is determined via the `strlen` function.
 *
 * \memberof ht_hash_table
 * \param ht The table to remove from
 * \param key The key to remove
 */
char *ht_remove_get(ht_hash_table *ht, const char *key);

#ifndef MAKE_DOCS
__attribute__((nonnull(1), nothrow, pure))
#endif
/**
 * \brief Creates an iterator over `ht`.
 *
 * Creates and returns an iterator over `ht`. The iterator can be advanced with
 * either `ht_iter_next` or `ht_iter_next_pair`.
 * Note that using any operations which may reallocate the buffer will
 * invalidate the iterator; this includes `ht_resize`, `ht_shrink_to_fit`, and
 * both `ht_insert` functions. `const` operations are safe to use, as well as
 * (in the current implementation) `ht_remove`.
 *
 * \memberof ht_hash_table
 * \param ht The table to iterate over
 */
ht_iter ht_iterator(const ht_hash_table *ht);

#ifndef MAKE_DOCS
__attribute__((nonnull(1), nothrow))
#endif
/**
 * \brief Returns the next key from `iter`.
 *
 * Advances `iter` and returns the next key, or `NULL` if the iterator was
 * empty.
 *
 * \memberof ht_hash_table
 * \param iter The iterator to advance
 */
char *ht_iter_next(ht_iter *iter);

#ifndef MAKE_DOCS
__attribute__((nonnull(1), nothrow))
#endif
/**
 * \brief Gets the next pair from `iter`.
 *
 * Advances `iter` and returns `true` if there was another pair, or `false if
 * the iterator was empty. If passed a non-`NULL` value, `key` and `val` will be
 * set to the key and value of the next pair, respectively; if there was no next
 * pair, `key` and `val` are unchanged.
 *
 * \memberof ht_hash_table
 * \param iter The iterator to advance
 * \param key If non-`NULL`, is set to the pair's key
 * \param val If non-`NULL`, is set to the pair's value
 */
bool ht_iter_next_pair(ht_iter *iter, char **key, char **val);

#ifndef MAKE_DOCS
__attribute__((nonnull(1), nothrow))
#endif
/**
 * \brief Creates a JSON string of `ht`.
 *
 * Creates a JSON string containing the pairs in `ht` and writes it to `out`,
 * overwriting any existing value. Allocates a buffer to hold it; the caller is
 * responsible for freeing `out`. (If `NULL` is passed to `out`, no buffer is
 * allocated and `out` isn't written to.) Returns the number of bytes written.
 * On a failure, 0 is returned, and `out` isn't changed, nor is any memory
 * allocated. Note that 0 will never be returned in a successful case, since the
 * opening & closing braces ensure a valid JSON string will never be 0 bytes.
 * Note also that this function assumes keys and values are valid JSON strings
 * (e.g. they don't contain unescaped quotes or invalid escape sequences), so
 * the function doesn't contain any logic to escape characters. If keys or
 * values may not hold valid JSON strings, use `ht_json_stringify_escape`
 * instead.
 *
 * \memberof ht_hash_table
 * \param ht The table to stringify
 * \param out Where to place the created buffer
 */
size_t ht_json_stringify(const ht_hash_table *ht, char **out);

#ifndef MAKE_DOCS
__attribute__((nonnull(1), nothrow))
#endif
/**
 * \brief Creates a JSON string of `ht`, escaping quotes.
 *
 * Creates a JSON string containing the pairs in `ht` and writes it to `out`,
 * overwriting any existing value. Allocates a buffer to hold it; the caller is
 * responsible for freeing `out`. (If `NULL` is passed to `out`, no buffer is
 * allocated and `out` isn't written to.) Returns the number of bytes written.
 * On a failure, 0 is returned, and `out` isn't changed, nor is any memory
 * allocated. Note that 0 will never be returned in a successful case, since the
 * opening & closing braces ensure a valid JSON string will never be 0 bytes.
 * Unlike `ht_json_stringify`, this function escapes quotes in keys and values
 * with a backslash, keeping it valid JSON. This function will be slightly
 * slower even without any quotes to escape, so if you know there won't be
 * quotes in keys or values, `ht_json_stringify` should be used.
 *
 * \memberof ht_hash_table
 * \param ht The table to stringify
 * \param out Where to place the created buffer
 */
size_t ht_json_stringify_escape(const ht_hash_table *ht, char **out);

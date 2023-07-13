#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

#include "ht-hash.h"

int main(int argc, char *argv[]) {
	(void) argc; (void) argv;

	ht_hash_table table = {0};

	// can add and retrieve keys
	ht_insert(&table, "hello", "world");
	assert(ht_contains(&table, "hello"));
	assert(strncmp("world", ht_search(&table, "hello"), 5) == 0);

	// can remove keys
	ht_remove(&table, "hello");
	assert(!ht_contains(&table, "hello"));

	// can reassign to keys
	ht_insert(&table, "hello", "George");
	ht_insert(&table, "hello", "Steve");
	assert(strncmp("Steve", ht_search(&table, "hello"), 5) == 0);

	// can resize tables
	size_t old_cap = table.capacity;
	ht_resize(&table, old_cap + 1);
	assert(table.capacity == old_cap << 1);

	// table can handle multiple values
	ht_insert(&table, "hello", "George");
	ht_insert(&table, "test", "key");
	assert(strncmp("George", ht_search(&table, "hello"), 6) == 0);
	assert(strncmp("key", ht_search(&table, "test"), 3) == 0);

	// table automatically resizes as needed
	char base_buf[8] = { 'k', 'e', 'y', 0, 0, 0, 0, 0 };
	char val_buf[8] = { 'v', 'a', 'l', 0, 0, 0, 0, 0 };
	for (int i = 0; i < 1000; i++) {
		sprintf(base_buf + 3, "%d", i);
		sprintf(val_buf + 3, "%d", i);
		ht_insert_unique(&table, base_buf, val_buf);
	}
	assert(strncmp("val0", ht_search(&table, "key0"), 4) == 0);
	assert(strncmp("val999", ht_search(&table, "key999"), 6) == 0);

	// iterators return all keys, without duplicates (order unknown)
	bool seenHello = false;
	bool seenTest = false;
	bool seenNums[1000] = {0};
	ht_iter iter = ht_iterator(&table);
	char *key;
	while ((key = ht_iter_next(&iter)) != NULL) {
		if (strncmp("hello", key, 5) == 0) {
			assert(!seenHello);
			seenHello = true;
		} else if (strncmp("test", key, 4) == 0) {
			assert(!seenTest);
			seenTest = true;
		} else if (strncmp("key", key, 3) == 0) {
			int idx = atoi(key + 3);
			assert(!seenNums[idx]);
			seenNums[idx] = true;
		}
	}
	assert(seenHello);
	assert(seenTest);
	for (int i = 0; i < 1000; i++) {
		assert(seenNums[i]);
	}

	// JSON stringifies correctly
	// really, there should be a snapshot or similar to confirm this
	char *buf_unescaped, *buf_escaped;
	size_t unescaped_len, escaped_len;
	unescaped_len = ht_json_stringify(&table, &buf_unescaped);
	assert(unescaped_len != 0);
	escaped_len = ht_json_stringify_escape(&table, &buf_escaped);
	assert(escaped_len != 0);
	assert(escaped_len == unescaped_len);
	assert(strncmp(buf_unescaped, buf_escaped, escaped_len) == 0);
	free(buf_unescaped);
	free(buf_escaped);

	// escaped JSON is different when a value contains a quote
	ht_insert(&table, "test", "escaped \"");
	unescaped_len = ht_json_stringify(&table, &buf_unescaped);
	assert(unescaped_len != 0);
	escaped_len = ht_json_stringify_escape(&table, &buf_escaped);
	assert(escaped_len != 0);
	assert(escaped_len == unescaped_len + 1);
	assert(strncmp(buf_unescaped, buf_escaped, unescaped_len) != 0);
	free(buf_unescaped);
	free(buf_escaped);

	// only uses the first `n` bytes when specified
	ht_insertn(&table, "testkey", 4, "hello there", 5);
	assert(strncmp(ht_search(&table, "test"), "hello", 5) == 0);
	assert(ht_containsn(&table, "test_", 4));
	ht_removen(&table, "test", 4);
	assert(!ht_contains(&table, "test"));

	// clearing a table removes all keys
	ht_clear(&table);
	assert(!ht_contains(&table, "hello"));
	assert(table.capacity == 0);
	assert(table.size == 0);
	return 0;
}

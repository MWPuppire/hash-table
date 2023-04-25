#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "ht-hash.h"

int main(int argc, char *argv[]) {
	(void) argc; (void) argv;
	ht_hash_table *table = ht_new();
	// can add and retrieve keys
	ht_insert(table, "hello", "world");
	assert(ht_contains(table, "hello"));
	assert(strcmp("world", ht_search(table, "hello")) == 0);
	// can remove keys
	ht_remove(table, "hello");
	assert(!ht_contains(table, "hello"));
	// can reassign to keys
	ht_insert(table, "hello", "George");
	ht_insert(table, "hello", "Steve");
	assert(strcmp("Steve", ht_search(table, "hello")) == 0);
	// can resize tables
	size_t old_cap = table->capacity;
	ht_resize(table, old_cap + 1);
	assert(table->capacity == old_cap << 1);
	// table can handle multiple values
	ht_insert(table, "hello", "George");
	ht_insert(table, "test", "key");
	assert(strcmp("George", ht_search(table, "hello")) == 0);
	assert(strcmp("key", ht_search(table, "test")) == 0);
	// table automatically resizes as needed
	char base_buf[8] = { 'k', 'e', 'y', 0, 0, 0, 0, 0 };
	for (int i = 0; i < 1000; i++) {
		sprintf(base_buf + 3, "%d", i);
		ht_insert_unique(table, base_buf, base_buf);
	}
	assert(strcmp("key0", ht_search(table, "key0")) == 0);
	assert(strcmp("key999", ht_search(table, "key999")) == 0);

	return 0;
}

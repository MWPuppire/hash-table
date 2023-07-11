#include <catch2/catch_test_macros.hpp>

#include <sstream>

#include "hash-table.hpp"

TEST_CASE("new map is empty") {
	REQUIRE(HashTable<int>().empty());
	REQUIRE(HashTable<int>().max_size() == 0);
}

TEST_CASE("keys added can be retrieved") {
	HashTable<int> x;
	x.insert("foo", 3);
	REQUIRE(x.find("foo") == 3);
	REQUIRE(x["foo"] == 3);
	x["bar"] = 42;
	REQUIRE(x.find("bar") == 42);
	REQUIRE(x.size() == 2);
}

TEST_CASE("can reassign to keys") {
	HashTable<int> x;
	x.insert("foo", 3);
	REQUIRE(x.find("foo") == 3);
	x.insert("foo", 42);
	REQUIRE(x.find("foo") == 42);
	x["foo"] = 255;
	REQUIRE(x.find("foo") == 255);
}

TEST_CASE("keys can be removed") {
	HashTable<int> x;
	x.insert("foo", 42);
	REQUIRE(x.find("foo") == 42);
	x.remove("foo");
	REQUIRE(!x.contains("foo"));
	x.insert("foo", 255);
	REQUIRE(x.find("foo") == 255);
}

TEST_CASE("many keys can be handled") {
	HashTable<int> x;
	std::ostringstream key_stream;
	for (int i = 0; i < 1000; i++) {
		key_stream.str("");
		key_stream << "key" << i;
		x.insert(key_stream.str(), i);
	}
	REQUIRE(x.find("key0") == 0);
	REQUIRE(x.find("key999") == 999);
	REQUIRE(x.size() == 1000);
}

TEST_CASE("can resize tables") {
	HashTable<int> x;
	x.reserve(3);
	size_t current = x.max_size();
	REQUIRE(current >= 3);
	x.reserve(current * 2);
	REQUIRE(x.max_size() > current);
}

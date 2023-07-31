#include <catch2/catch_test_macros.hpp>

#include <string>
#include <sstream>
#include <iostream>

#include "hash-table.hpp"

TEST_CASE("new map is empty") {
	REQUIRE(HashTable<int>().empty());
	REQUIRE(HashTable<int>().max_size() == 0);
}

TEST_CASE("keys added can be retrieved") {
	HashTable<int> x;
	x.insert("foo", 3);
	REQUIRE((*x.find("foo")).second == 3);
	REQUIRE(x["foo"] == 3);
	x["bar"] = 42;
	REQUIRE((*x.find("bar")).second == 42);
	REQUIRE(x.size() == 2);
}

TEST_CASE("can reassign to keys") {
	HashTable<int> x;
	x.insert("foo", 3);
	REQUIRE((*x.find("foo")).second == 3);
	x.insert_or_assign("foo", 42);
	REQUIRE((*x.find("foo")).second == 42);
	x["foo"] = 255;
	REQUIRE((*x.find("foo")).second == 255);
}

TEST_CASE("keys can be removed") {
	HashTable<int> x;
	x.insert("foo", 42);
	REQUIRE((*x.find("foo")).second == 42);
	x.erase("foo");
	REQUIRE(!x.contains("foo"));
	x.insert("foo", 255);
	REQUIRE((*x.find("foo")).second == 255);
}

TEST_CASE("many keys can be handled") {
	HashTable<int> x;
	std::ostringstream key_stream;
	for (int i = 0; i < 1000; i++) {
		key_stream.str("");
		key_stream << "key" << i;
		x.insert(key_stream.str(), i);
	}
	REQUIRE((*x.find("key0")).second == 0);
	REQUIRE((*x.find("key999")).second == 999);
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

TEST_CASE("can iterate over key/value pairs") {
	HashTable<int> x;
	std::ostringstream key_stream;
	for (int i = 0; i < 1000; i++) {
		key_stream.str("");
		key_stream << "key" << i;
		x.insert_or_assign(key_stream.str(), i);
	}
	size_t pairs = 0;
	for (const auto &pair : x) {
		const auto &key = pair.first;
		REQUIRE(key.substr(0, 3) == "key");
		REQUIRE(pair.second == std::stoi(key.substr(3)));
		pairs++;
	}
	REQUIRE(pairs == 1000);
}

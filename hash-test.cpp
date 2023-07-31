#include <catch2/catch_test_macros.hpp>

#include <string>
#include <iostream>

#include "hash-table.hpp"

TEST_CASE("new map is empty") {
	REQUIRE(HashTable<std::string, int>().empty());
	REQUIRE(HashTable<std::string, int>().max_size() == 0);
}

TEST_CASE("keys added can be retrieved") {
	HashTable<std::string, int> x;
	x.insert("foo", 3);
	REQUIRE((*x.find("foo")).second == 3);
	REQUIRE(x["foo"] == 3);
	x["bar"] = 42;
	REQUIRE((*x.find("bar")).second == 42);
	REQUIRE(x.size() == 2);
}

TEST_CASE("can reassign to keys") {
	HashTable<std::string, int> x;
	x.insert("foo", 3);
	REQUIRE((*x.find("foo")).second == 3);
	x.insert_or_assign("foo", 42);
	REQUIRE((*x.find("foo")).second == 42);
	x["foo"] = 255;
	REQUIRE((*x.find("foo")).second == 255);
}

TEST_CASE("keys can be removed") {
	HashTable<std::string, int> x;
	x.insert("foo", 42);
	REQUIRE((*x.find("foo")).second == 42);
	x.erase("foo");
	REQUIRE(!x.contains("foo"));
	x.insert("foo", 255);
	REQUIRE((*x.find("foo")).second == 255);
}

TEST_CASE("many keys can be handled") {
	HashTable<std::string, int> x;
	for (int i = 0; i < 1000; i++) {
		std::string key = "key";
		key += std::to_string(i);
		x.insert(key, i);
	}
	REQUIRE((*x.find("key0")).second == 0);
	REQUIRE((*x.find("key999")).second == 999);
	REQUIRE(x.size() == 1000);
}

TEST_CASE("can resize tables") {
	HashTable<std::string, int> x;
	x.reserve(3);
	size_t current = x.max_size();
	REQUIRE(current >= 3);
	x.reserve(current * 2);
	REQUIRE(x.max_size() > current);
	// since there's no keys, the minimum capacity is 0
	x.shrink_to_fit();
	REQUIRE(x.max_size() == 0);
}

TEST_CASE("can iterate over key/value pairs") {
	HashTable<std::string, int> x;
	for (int i = 0; i < 1000; i++) {
		std::string key = "key";
		key += std::to_string(i);
		x.insert_or_assign(key, i);
	}
	bool found[1000] = { 0 };
	size_t pairs = 0;
	for (const auto &pair : x) {
		const auto &key = pair.first;
		const auto val = pair.second;
		REQUIRE(!found[val]);
		REQUIRE(key.substr(0, 3) == "key");
		REQUIRE(val == std::stoi(key.substr(3)));
		pairs++;
		found[val] = true;
	}
	REQUIRE(pairs == 1000);
}

TEST_CASE("copy constructor creates identical map") {
	HashTable<std::string, int> x;
	for (int i = 0; i < 1000; i++) {
		std::string key = "key";
		key += std::to_string(i);
		x.insert_or_assign(key, i);
	}
	HashTable<std::string, int> y = x;
	REQUIRE(x == y);
}

TEST_CASE("equality operator works correctly") {
	HashTable<std::string, int> x;
	x["a"] = 9;
	x["b"] = 11;
	REQUIRE(x == x);
	HashTable<std::string, int> y = x;
	REQUIRE(x == y);
	HashTable<std::string, int> z;
	z["a"] = 9;
	z["b"] = 11;
	REQUIRE(x == z);
	y["c"] = 14;
	REQUIRE(x != y);
	z["b"] = 12;
	REQUIRE(x != z);
}

TEST_CASE("move constructor destroys original") {
	HashTable<std::string, int> x;
	for (int i = 0; i < 1000; i++) {
		std::string key = "key";
		key += std::to_string(i);
		x.insert_or_assign(key, i);
	}
	HashTable<std::string, int> y = x;
	HashTable<std::string, int> z = std::move(x);
	REQUIRE(x != z);
	REQUIRE(y == z);
	REQUIRE(x.empty());
}

TEST_CASE("erasing items by iterator works") {
	HashTable<std::string, int> x;
	for (int i = 0; i < 1000; i++) {
		std::string key = "key";
		key += std::to_string(i);
		x.insert_or_assign(key, i);
	}
	auto iter = x.find("key400");
	x.erase(iter);
	REQUIRE(x.size() == 999);
	REQUIRE(!x.contains("key400"));
	x.erase(x.begin(), x.end());
	REQUIRE(x.empty());
}

TEST_CASE("at throws if key isn't found") {
	HashTable<std::string, int> x;
	x["a"] = 3;
	x["b"] = 4;
	x["c"] = 5;
	REQUIRE(x.at("a") == 3);
	REQUIRE(x.at("b") == 4);
	REQUIRE(x.at("c") == 5);
	try {
		x.at("d") = 6;
		REQUIRE(false);
	} catch (std::out_of_range& err) {
		REQUIRE(true);
	}
}

TEST_CASE("swap moves keys and values") {
	HashTable<std::string, int> x, y;
	x["a"] = 3;
	x["b"] = 6;
	x["c"] = 10;
	y["a"] = 9;
	y["b"] = 14;
	y["d"] = 16;
	x.swap(y);
	REQUIRE(x.contains("d"));
	REQUIRE(!y.contains("d"));
	REQUIRE(x["a"] == 9);
	REQUIRE(y["a"] == 3);
}

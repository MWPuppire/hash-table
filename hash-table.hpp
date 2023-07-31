#include <string>
#include <stdexcept>
#include <utility>
#include <type_traits>
#include <optional>
#include <functional>
#include <ostream>

template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
class HashTable {
public:
	using key_type = Key;
	using mapped_type = T;
	using value_type = std::pair<const Key, T>;
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	class iterator;
	class const_iterator;
private:
	using HtItem = std::optional<std::pair<const Key, T>>;

	static const size_t FIB_MULT = 11400714819323198485ull;
	static const size_t HT_PRIME = 151;
	static const size_t INITIAL_CAPACITY = 32;

	size_t capacity;
	size_t len;
	HtItem* items;

	using HashNothrow = std::conjunction<
		std::is_nothrow_default_constructible<Hash>,
		std::is_nothrow_invocable_r<size_t, Hash, const Key&>
	>;
	using IndexNothrow = std::conjunction<
		HashTable::HashNothrow,
		std::is_nothrow_default_constructible<KeyEqual>,
		std::is_nothrow_invocable_r<bool, KeyEqual, const Key&, const Key&>
	>;
	using ItemNothrowDefault = std::conjunction<
		std::is_nothrow_default_constructible<Key>,
		std::is_nothrow_default_constructible<T>
	>;
	using ItemNothrowCopy = std::conjunction<
		std::is_nothrow_copy_constructible<Key>,
		std::is_nothrow_copy_constructible<T>
	>;
	using ItemNothrowMove = std::conjunction<
		std::is_nothrow_move_constructible<Key>,
		std::is_nothrow_move_constructible<T>
	>;
	using ItemNothrowDestructible = std::conjunction<
		std::is_nothrow_destructible<Key>,
		std::is_nothrow_destructible<T>
	>;

	static size_t hash(const Key& val, size_t cap) noexcept(HashNothrow::value) {
		int shift = 64 - __builtin_ctz(cap);
		size_t hash = Hash{}(val);
		return (hash * HashTable::FIB_MULT) >> shift;
	}

	std::pair<bool, size_t> index_of(const Key& key) const noexcept(IndexNothrow::value) {
		size_t cap = this->capacity;
		if (cap == 0) {
			return std::make_pair(false, 0);
		}
		size_t index = HashTable::hash(key, cap);
		size_t attempts = 0;
		KeyEqual cmp{};
		while (this->items[index]) {
			if (cmp((*this->items[index]).first, key)) {
				return std::make_pair(true, index);
			}
			index = (index + 1) & (cap - 1);
			if (attempts > this->len) {
				return std::make_pair(false, index);
			}
		}
		return std::make_pair(false, index);
	}

	// Assumes `key` does not already exist in `items`, so it continues to
	// hash as long as that slot is occupied.
	static std::pair<iterator, bool> inner_insert(HtItem* items, size_t cap, value_type pair) noexcept(HashNothrow::value) {
		size_t index = HashTable::hash(pair.first, cap);
		while (items[index]) {
			index = (index + 1) & (cap - 1);
		}
		items[index].emplace(std::move(pair));
		return std::make_pair(iterator(items + index), true);
	}

	template<class... Args>
	std::pair<iterator, bool> emplace_unique_hint(iterator hint, Args&&... args) noexcept(IndexNothrow::value && ItemNothrowMove::value && std::is_nothrow_invocable<value_type, Args...>::value) {
		value_type pair(std::forward<Args>(args)...);
		KeyEqual cmp{};
		if (hint != this->end() && (!hint.item->has_value() || cmp((*hint).first, pair.first))) {
			bool had_value = hint.item->has_value();
			hint.item->emplace(std::move(pair));
			if (!had_value) {
				size_t cap = this->capacity;
				// test for resize here; still 75% capacity
				if (this->len++ << 2 > (cap << 1) + cap) {
					this->reserve_exact(cap, cap << 1);
				}
			}
			return std::make_pair(hint, !had_value);
		}
		if (this->capacity == 0) {
			this->reserve_exact(0, HashTable::INITIAL_CAPACITY);
		}
		size_t cap = this->capacity;
		// resize on 75% capacity
		if (this->len++ << 2 > (cap << 1) + cap) {
			this->reserve_exact(cap, cap << 1);
			return HashTable::inner_insert(this->items, cap << 1, std::move(pair));
		} else {
			return HashTable::inner_insert(this->items, cap, std::move(pair));
		}
	}

	// `new_cap` must be a power of 2.
	void reserve_exact(size_t old_cap, size_t new_cap) noexcept(HashNothrow::value && ItemNothrowMove::value) {
		HtItem* new_items = new HtItem[new_cap]{};
		try {
			for (size_t i = 0; i < old_cap; i++) {
				if (this->items[i]) {
					HashTable::inner_insert(
						new_items,
						new_cap,
						std::move(*this->items[i])
					);
				}
			}
		} catch (...) {
			delete[] new_items;
			throw;
		}
		delete[] this->items;
		this->items = new_items;
		this->capacity = new_cap;
	}

public:
	class iterator {
		friend class HashTable;
	private:
		HtItem* item;
		const HtItem* end;
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = value_type;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type*;
		using reference = value_type&;
		iterator(HtItem* item) noexcept : item(item), end(item) { }
		iterator(HtItem* item, const HtItem* end) noexcept : end(end) {
			while (!item->has_value() && item != end) {
				item++;
			}
			this->item = item;
		}
		iterator& operator++() noexcept {
			if (this->item == this->end) {
				return *this;
			}
			do {
				this->item++;
			} while (!this->item->has_value() && this->item != this->end);
			return *this;
		}
		iterator operator++(int) noexcept {
			iterator out = *this;
			++(*this);
			return out;
		}
		bool operator==(const iterator& other) const noexcept {
			return this->item == other.item && this->end == other.end;
		}
		bool operator!=(const iterator& other) const noexcept {
			return !(*this == other);
		}
		reference operator*() {
			return this->item->value();
		}
		operator const_iterator() const {
			return const_iterator(this->item, this->end);
		}
	};
	class const_iterator {
		friend class HashTable;
	private:
		const HtItem* item;
		const HtItem* end;
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = value_type;
		using difference_type = std::ptrdiff_t;
		using pointer = const value_type*;
		using reference = const value_type&;
		const_iterator(const HtItem* item) noexcept : item(item), end(item) { }
		const_iterator(const HtItem* item, const HtItem* end) noexcept : end(end) {
			while (!item->has_value() && item != end) {
				item++;
			}
			this->item = item;
		}
		const_iterator& operator++() noexcept {
			if (this->item == this->end) {
				return *this;
			}
			do {
				this->item++;
			} while (!this->item->has_value() && this->item != this->end);
			return *this;
		}
		const_iterator operator++(int) noexcept {
			iterator out = *this;
			++(*this);
			return out;
		}
		bool operator==(const const_iterator& other) const noexcept {
			return this->item == other.item && this->item == other.end;
		}
		bool operator!=(const const_iterator& other) const noexcept {
			return !(*this == other);
		}
		const_reference operator*() const {
			return this->item->value();
		}
	};

	HashTable() noexcept {
		this->capacity = this->len = 0;
		this->items = nullptr;
	}
	// Copy constructor
	HashTable(const HashTable& other) noexcept(ItemNothrowCopy::value) {
		if (other.capacity == 0) {
			this->capacity = this->len = 0;
			this->items = nullptr;
			return;
		}
		this->capacity = other.capacity;
		this->len = other.len;
		this->items = new HtItem[other.capacity]{};
		try {
			for (size_t i = 0; i < other.capacity; i++) {
				if (other.items[i]) {
					this->items[i].emplace((*other.items[i]).first, (*other.items[i]).second);
				}
			}
		} catch (...) {
			delete[] this->items;
			throw;
		}
	}
	// Move constructor
	HashTable(HashTable&& other) noexcept {
		this->capacity = other.capacity;
		this->len = other.len;
		this->items = other.items;
		other.capacity = other.len = 0;
		other.items = nullptr;
	}
	~HashTable() noexcept(ItemNothrowDestructible::value) {
		delete[] this->items;
	}

	// Copy assignment
	HashTable& operator=(const HashTable& other) noexcept(ItemNothrowCopy::value && ItemNothrowDestructible::value) {
		if (this == &other) {
			return *this;
		}
		HtItem* new_items = new HtItem[other.capacity]{};
		try {
			for (size_t i = 0; i < other.capacity; i++) {
				if (other.items[i]) {
					new_items[i].emplace((*other.items[i]).first, (*other.items[i]).second);
				}
			}
		} catch (...) {
			delete[] new_items;
			throw;
		}
		delete[] this->items;
		this->capacity = other.capacity;
		this->len = other.len;
		this->items = new_items;
		return *this;
	}
	// Move assignment
	HashTable& operator=(HashTable&& other) noexcept {
		if (this == &other) {
			return *this;
		}
		this->capacity = other.capacity;
		this->len = other.len;
		this->items = other.items;
		other.capacity = other.len = 0;
		other.items = nullptr;
		return *this;
	}

	bool operator==(const HashTable& other) const noexcept(IndexNothrow::value && std::is_nothrow_invocable_r<bool, decltype(std::declval<T>() == std::declval<T>()), const T&, const T&>::value) {
		if (this->len != other.len) {
			return false;
		}
		// test pointer equality before doing the more expensive test
		if (this->items == other.items) {
			return true;
		}
		for (const auto& [key, val] : *this) {
			const_iterator iter = other.find(key);
			if (iter == other.cend() || val != (*iter).second) {
				return false;
			}
		}
		return true;
	}
	bool operator!=(const HashTable& other) const noexcept(IndexNothrow::value && std::is_nothrow_invocable_r<bool, decltype(std::declval<T>() == std::declval<T>()), const T&, const T&>::value) {
		return !(*this == other);
	}

	bool contains(const Key& key) const noexcept(IndexNothrow::value) {
		return this->index_of(key).first;
	}
	size_t count(const Key& key) const noexcept(IndexNothrow::value) {
		return this->index_of(key).first ? 1 : 0;
	}

	// Because this specifies only a minimum capacity (without an upper
	// bound), it never lowers capacity, assuming that if that many items
	// were ever allocated, that many may be allocated again later. If
	// lowering memory usage is desired, use `shrink_to_fit`.
	void reserve(size_t min_capacity) noexcept(HashNothrow::value && ItemNothrowMove::value) {
		size_t old_cap = this->capacity;
		if (min_capacity <= old_cap) {
			return;
		}

		// round up to next power of 2
		size_t new_cap = min_capacity;
		new_cap--;
		new_cap |= new_cap >> 1;
		new_cap |= new_cap >> 2;
		new_cap |= new_cap >> 4;
		new_cap |= new_cap >> 8;
		new_cap |= new_cap >> 16;
		new_cap |= new_cap >> 32;
		new_cap++;

		this->reserve_exact(old_cap, new_cap);
	}

	void shrink_to_fit() noexcept(HashNothrow::value && ItemNothrowMove::value) {
		if (this->len == 0) {
			this->clear();
			return;
		}
		size_t old_cap = this->capacity;
		size_t new_cap = this->len;
		new_cap--;
		new_cap |= new_cap >> 1;
		new_cap |= new_cap >> 2;
		new_cap |= new_cap >> 4;
		new_cap |= new_cap >> 8;
		new_cap |= new_cap >> 16;
		new_cap |= new_cap >> 32;
		new_cap++;
		if (new_cap >= old_cap) {
			return;
		}
		this->reserve_exact(old_cap, new_cap);
	}

	std::pair<iterator, bool> insert(const Key& key, const T& value) noexcept(IndexNothrow::value && ItemNothrowMove::value && ItemNothrowCopy::value) {
		return this->emplace(Key{key}, T{value});
	}
	std::pair<iterator, bool> insert(Key&& key, T&& value) noexcept(IndexNothrow::value && ItemNothrowMove::value) {
		return this->emplace(std::move(key), std::move(value));
	}
	std::pair<iterator, bool> insert_or_assign(const Key& key, const T& value) noexcept(IndexNothrow::value && ItemNothrowMove::value && ItemNothrowCopy::value) {
		return this->emplace_or_assign(Key{key}, T{value});
	}
	std::pair<iterator, bool> insert_or_assign(Key&& key, T&& value) noexcept(IndexNothrow::value && ItemNothrowMove::value) {
		return this->emplace_or_assign(std::move(key), std::move(value));
	}
	std::pair<iterator, bool> insert_unique(const Key& key, const T& value) noexcept(HashNothrow::value && ItemNothrowMove::value && ItemNothrowCopy::value) {
		return this->insert_unique(Key{key}, T{value});
	}
	std::pair<iterator, bool> insert_unique(Key&& key, T&& value) noexcept(HashNothrow::value && ItemNothrowMove::value) {
		if (this->capacity == 0) {
			this->reserve_exact(0, HashTable::INITIAL_CAPACITY);
		}
		value_type pair(std::move(key), std::move(value));
		size_t cap = this->capacity;
		// resize on 75% capacity
		if (this->len++ << 2 > (cap << 1) + cap) {
			this->reserve_exact(cap, cap << 1);
			return HashTable::inner_insert(this->items, cap << 1, std::move(pair));
		} else {
			return HashTable::inner_insert(this->items, cap, std::move(pair));
		}
	}

	template<class... Args>
	std::pair<iterator, bool> emplace(Args&&... args) noexcept(IndexNothrow::value && ItemNothrowMove::value && std::is_nothrow_invocable<value_type, Args...>::value) {
		value_type pair(std::forward<Args>(args)...);
		auto [contains, cur_index] = this->index_of(pair.first);
		if (contains) {
			return std::make_pair(iterator(this->items + cur_index), false);
		}
		return this->emplace_unique_hint(iterator(this->items + cur_index), std::move(pair));
	}

	template<class... Args>
	std::pair<iterator, bool> emplace_hint(const_iterator hint, Args&&... args) noexcept(IndexNothrow::value && ItemNothrowMove::value && std::is_nothrow_invocable<value_type, Args...>::value) {
		value_type pair(std::forward<Args>(args)...);
		if (hint != this->cend()) {
			HtItem* item = hint.item;
			bool had_value = item->has_value();
			if (!had_value) {
				this->len++;
				item->emplace(std::move(pair));
			}
			return std::make_pair(iterator(item), !had_value);
		}
		// hint was bad, ignore it
		return this->emplace(std::move(pair));
	}

	template<class... Args>
	std::pair<iterator, bool> emplace_or_assign(Args&&... args) noexcept(IndexNothrow::value && ItemNothrowMove::value && std::is_nothrow_invocable<value_type, Args...>::value) {
		value_type pair(std::forward<Args>(args)...);
		auto [contains, cur_index] = this->index_of(pair.first);
		if (contains) {
			this->items[cur_index].emplace(std::move(pair));
			return std::make_pair(iterator(this->items + cur_index), false);
		}
		return this->emplace_unique_hint(iterator(this->items + cur_index), std::move(pair));
	}

	iterator find(const Key& key) noexcept(IndexNothrow::value) {
		auto [contains, index] = this->index_of(key);
		if (contains) {
			return iterator(this->items + index);
		} else {
			return this->end();
		}
	}
	const_iterator find(const Key& key) const noexcept(IndexNothrow::value) {
		auto [contains, index] = this->index_of(key);
		if (contains) {
			return const_iterator(this->items + index);
		} else {
			return this->cend();
		}
	}
	T& find_or_insert(const Key& key) noexcept(IndexNothrow::value && ItemNothrowDefault::value) {
		if (this->capacity == 0) {
			this->reserve_exact(0, HashTable::INITIAL_CAPACITY);
		}
		auto [contains, index] = this->index_of(key);
		if (contains) {
			return (*this->items[index]).second;
		} else {
			this->len++;
			this->items[index].emplace(key, T{});
			return (*this->items[index]).second;
		}
	}

	T& operator[](const Key& key) noexcept(IndexNothrow::value && ItemNothrowDefault::value) {
		return this->find_or_insert(key);
	}

	T& at(const Key& key) {
		auto [contains, index] = this->index_of(key);
		if (contains) {
			return (*this->items[index]).second;
		} else {
			throw std::out_of_range("Key doesn't exist");
		}
	}
	const T& at(const Key& key) const {
		auto [contains, index] = this->index_of(key);
		if (contains) {
			return (*this->items[index]).second;
		} else {
			throw std::out_of_range("Key doesn't exist");
		}
	}

	iterator erase(iterator pos) noexcept(ItemNothrowDestructible::value) {
		pos.item->reset();
		this->len--;
		return ++pos;
	}
	// sort-of cheating, but it works
	iterator erase(const_iterator pos) noexcept(ItemNothrowDestructible::value) {
		((HtItem*) (pos.item))->reset();
		this->len--;
		return ++pos;
	}
	iterator erase(const_iterator first, const_iterator last) noexcept(ItemNothrowDestructible::value) {
		while (first != last) {
			((HtItem*) (first.item))->reset();
			this->len--;
			++first;
		}
		return iterator((HtItem*) last.item, last.end);
	}
	size_t erase(const Key& key) noexcept(IndexNothrow::value && ItemNothrowDestructible::value) {
		auto [contains, index] = this->index_of(key);
		if (contains) {
			this->items[index].reset();
			this->len--;
			return 1;
		} else {
			return 0;
		}
	}

	void clear() noexcept(ItemNothrowDestructible::value) {
		delete[] this->items;
		this->capacity = this->len = 0;
		this->items = nullptr;
	}

	void swap(HashTable<Key, T>& other) noexcept {
		auto items = this->items;
		auto capacity = this->capacity;
		auto len = this->len;
		this->items = other.items;
		this->capacity = other.capacity;
		this->len = other.len;
		other.items = items;
		other.capacity = capacity;
		other.len = len;
	}

	bool empty() const noexcept {
		return this->len == 0;
	}
	size_t size() const noexcept {
		return this->len;
	}
	size_t max_size() const noexcept {
		return this->capacity;
	}

	iterator begin() {
		return iterator(this->items, this->items + this->capacity);
	}
	iterator end() {
		return iterator(this->items + this->capacity);
	}
	const_iterator begin() const {
		return const_iterator(this->items, this->items + this->capacity);
	}
	const_iterator end() const {
		return const_iterator(this->items + this->capacity);
	}
	const_iterator cbegin() const {
		return const_iterator(this->items, this->items + this->capacity);
	}
	const_iterator cend() const {
		return const_iterator(this->items + this->capacity);
	}
};
template<class Key, class T>
void swap(HashTable<Key, T>& h1, HashTable<Key, T>& h2) {
	h1.swap(h2);
}

template<class Key, class T>
std::ostream& operator<<(std::ostream& os, const HashTable<Key, T>& table) {
	if (table.size() == 0) {
		os << "HashTable {}";
		return os;
	}
	os << "HashTable {" << std::endl;
	for (const auto& [key, val] : table) {
		os << "\t" << key << ": " << val << "," << std::endl;
	}
	os << "}";
	return os;
}

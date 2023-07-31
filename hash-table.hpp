#include <string>
#include <stdexcept>
#include <utility>
#include <type_traits>
#include <optional>

static const size_t FIB_MULT = 11400714819323198485ull;
static const size_t HT_PRIME = 151;
static const size_t INITIAL_CAPACITY = 32;

static size_t ht_hash(const std::string& s, size_t cap) noexcept {
	int shift = 64 - __builtin_ctz(cap);
	size_t hash = 0;
	size_t len = s.size();
	size_t mult = 1;
	for (size_t i = 0; i < len; i++) {
		hash += mult * s[i];
		mult *= HT_PRIME;
	}
	return (hash * FIB_MULT) >> shift;
}

template<typename T>
class HashTable {
private:
	typedef std::optional<std::pair<const std::string, T>> HtItem;

	size_t capacity;
	size_t len;
	HtItem *items;

	bool index_of(const std::string& key, size_t& out_index) const noexcept {
		size_t cap = this->capacity;
		if (cap == 0) {
			return false;
		}
		size_t index = ht_hash(key, cap);
		size_t attempts = 0;
		while (this->items[index]) {
			if ((*this->items[index]).first == key) {
				out_index = index;
				return true;
			}
			index = (index + (attempts << 1) + 1) & (cap - 1);
			if (attempts > this->len) {
				out_index = index;
				return false;
			}
		}
		out_index = index;
		return false;
	}

	// Assumes `key` does not already exist in `items`, so it continues to
	// hash as long as that slot is occupied.
	static void inner_insert(HtItem *items, size_t cap, std::string key, T value) noexcept {
		size_t index = ht_hash(key, cap);
		size_t attempts = 0;
		while (items[index]) {
			index = (index + (attempts++ << 1) + 1) & (cap - 1);
		}
		items[index].emplace(key, value);
	}

	// `new_cap` must be a power of 2.
	void reserve_exact(size_t old_cap, size_t new_cap) noexcept {
		HtItem *old_items = this->items;
		HtItem *new_items = new HtItem[new_cap]{};
		for (size_t i = 0; i < old_cap; i++) {
			if (old_items[i]) {
				HashTable<T>::inner_insert(new_items, new_cap, std::move((*old_items[i]).first), std::move((*old_items[i]).second));
			}
		}
		this->items = new_items;
		this->capacity = new_cap;
		delete[] old_items;
	}

public:
	typedef std::string key_type;
	typedef T mapped_type;
	typedef std::pair<const std::string, T> value_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef value_type &reference;
	typedef const value_type &const_reference;
	class iterator: public std::iterator<std::forward_iterator_tag, value_type, size_t, value_type *, value_type &> {
	private:
		HtItem *item;
		const HtItem *end;
	public:
		iterator(HtItem *item) noexcept : item(item), end(item) { }
		iterator(HtItem *item, const HtItem *end) noexcept : end(end) {
			while (!item && item != end) {
				item++;
			}
			this->item = item;
		}
		iterator &operator++() noexcept {
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
		bool operator==(const iterator &other) const noexcept {
			return this->item == other.item && this->end == other.end;
		}
		bool operator!=(const iterator &other) const noexcept {
			return this->item != other.item || this->end != other.end;
		}
		reference operator*() noexcept {
			return **this->item;
		}
	};
	class const_iterator: public std::iterator<std::forward_iterator_tag, value_type, size_t, const value_type *, const value_type &> {
	private:
		const HtItem *item;
		const HtItem *end;
	public:
		const_iterator(const HtItem *item) noexcept : item(item), end(item) { }
		const_iterator(const HtItem *item, const HtItem *end) noexcept : end(end) {
			while (!item && item != end) {
				item++;
			}
			this->item = item;
		}
		const_iterator &operator++() noexcept {
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
		bool operator==(const const_iterator &other) const noexcept {
			return this->item == other.item && this->item == other.end;
		}
		bool operator!=(const const_iterator &other) const noexcept {
			return this->item != other.item || this->item != other.end;
		}
		const_reference operator*() const noexcept {
			return **this->item;
		}
	};

	HashTable() noexcept {
		this->capacity = this->len = 0;
		this->items = nullptr;
	}
	// Copy constructor
	HashTable(const HashTable& other) noexcept(std::is_nothrow_copy_constructible<T>::value) {
		this->capacity = other.capacity;
		this->len = other.len;
		this->items = new HtItem[other.capacity]{};
		for (size_t i = 0; i < other.capacity; i++) {
			if (other.items[i]) {
				this->items[i].emplace(other.items[i].key, other.items[i].value);
			}
		}
	}
	// Move constructor
	HashTable(HashTable&& other) noexcept {
		this->capacity = other.capacity;
		this->len = other.len;
		this->items = std::move(other.items);
		other.capacity = other.len = 0;
		other.items = nullptr;
	}
	~HashTable() noexcept(std::is_nothrow_destructible<T>::value) {
		delete[] this->items;
	}

	// Copy assignment
	HashTable& operator=(const HashTable& other) noexcept(std::is_nothrow_copy_constructible<T>::value) {
		delete[] this->items;

		this->capacity = other.capacity;
		this->len = other.len;
		this->items = new HtItem[other.capacity];
		for (size_t i = 0; i < other.capacity; i++) {
			if (other.items[i]) {
				this->items[i].emplace(other.items[i].key, other.items[i].value);
			}
		}
	}
	// Move assignment
	HashTable& operator=(HashTable&& other) noexcept {
		this->capacity = other.capacity;
		this->len = other.len;
		this->items = std::move(other.items);
		other.capacity = other.len = 0;
		other.items = nullptr;
	}

	bool contains(const std::string& key) const noexcept {
		size_t drop;
		return this->index_of(key, drop);
	}

	void reserve(size_t min_capacity) noexcept {
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

	void insert(const std::string key, T value) noexcept(std::is_nothrow_copy_constructible<T>::value) {
		size_t cur_index;
		bool contains = this->index_of(key, cur_index);
		if (contains) {
			return;
		}
		if (this->capacity == 0) {
			this->reserve_exact(0, INITIAL_CAPACITY);
		}
		size_t cap = this->capacity;
		// resize on 75% capacity
		if (this->len++ << 2 > (cap << 1) + cap) {
			this->reserve_exact(cap, cap << 1);
			return HashTable<T>::inner_insert(this->items, cap << 1, key, T{value});
		} else {
			return HashTable<T>::inner_insert(this->items, cap, key, T{value});
		}
	}
	void insert_or_assign(const std::string key, T value) noexcept(std::is_nothrow_copy_constructible<T>::value) {
		size_t cur_index;
		bool contains = this->index_of(key, cur_index);
		if (contains) {
			(*this->items[cur_index]).second = T{value};
		}
		if (this->capacity == 0) {
			this->reserve_exact(0, INITIAL_CAPACITY);
		}
		size_t cap = this->capacity;
		// resize on 75% capacity
		if (this->len++ << 2 > (cap << 1) + cap) {
			this->reserve_exact(cap, cap << 1);
			return HashTable<T>::inner_insert(this->items, cap << 1, key, T{value});
		} else {
			return HashTable<T>::inner_insert(this->items, cap, key, T{value});
		}
	}
	void insert_unique(const std::string key, T value) noexcept(std::is_nothrow_copy_constructible<T>::value) {
		if (this->capacity == 0) {
			this->reserve_exact(0, INITIAL_CAPACITY);
		}
		size_t cap = this->capacity;
		// resize on 75% capacity
		if (this->len++ << 2 > (cap << 1) + cap) {
			this->reserve_exact(cap, cap << 1);
			return HashTable<T>::inner_insert(this->items, cap << 1, key, T{value});
		} else {
			return HashTable<T>::inner_insert(this->items, cap, key, T{value});
		}
	}

	const T& find(const std::string& key) const {
		size_t index;
		if (this->index_of(key, index)) {
			return (*this->items[index]).second;
		} else {
			throw std::out_of_range("no such key " + key);
		}
	}
	T& find(const std::string& key) {
		size_t index;
		if (this->index_of(key, index)) {
			return (*this->items[index]).second;
		} else {
			throw std::out_of_range("no such key " + key);
		}
	}
	std::optional<const T&> find_opt(const std::string& key) const noexcept {
		size_t index;
		if (this->index_of(key, index)) {
			return std::optional(*this->items[index].value);
		} else {
			return std::nullopt;
		}
	}
	std::optional<T&> find_opt(const std::string& key) noexcept {
		size_t index;
		if (this->index_of(key, index)) {
			return std::optional(*this->items[index].value);
		} else {
			return std::nullopt;
		}
	}

	T& find_or_insert(const std::string& key) noexcept(std::is_nothrow_default_constructible<T>::value) {
		if (this->capacity == 0) {
			this->reserve_exact(0, INITIAL_CAPACITY);
		}
		size_t index;
		if (this->index_of(key, index)) {
			return (*this->items[index]).second;
		} else {
			this->len++;
			this->items[index].emplace(key, T{});
			return (*this->items[index]).second;
		}
	}

	T& operator[](const std::string& key) noexcept(std::is_nothrow_default_constructible<T>::value) {
		return this->find_or_insert(key);
	}

	void remove(const std::string& key) noexcept(std::is_nothrow_destructible<T>::value) {
		size_t index;
		if (this->index_of(key, index)) {
			this->items[index].reset();
			this->len--;
		}
	}

	void clear() noexcept(std::is_nothrow_destructible<T>::value) {
		delete[] this->items;
		this->capacity = this->len = 0;
		this->items = nullptr;
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
	const_iterator cbegin() const {
		return const_iterator(this->items, this->items + this->capacity);
	}
	const_iterator cend() const {
		return const_iterator(this->items + this->capacity);
	}
};

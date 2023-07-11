#include <string>
#include <stdexcept>

#if __cplusplus >= 201703L
#	include <optional>
#endif

#if __cplusplus < 201103L
#	define nullptr 0
#	define noexcept(x)
#else
#	include <type_traits>
#endif

static const size_t FIB_MULT = 11400714819323198485ull;
static const size_t HT_PRIME = 151;
static const size_t INITIAL_CAPACITY = 32;

static size_t ht_hash(const std::string& s, size_t cap) noexcept(true) {
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
struct HtItem {
	std::string key;
	T *value;
	bool occupied;
};

template<typename T>
class HashTable {
private:
	size_t capacity;
	size_t len;
	HtItem<T> *items;

	bool index_of(const std::string& key, size_t& out_index) const noexcept(true) {
		size_t cap = this->capacity;
		if (cap == 0) {
			return false;
		}
		size_t index = ht_hash(key, cap);
		size_t attempts = 0;
		while (this->items[index].occupied) {
			if (this->items[index].key == key) {
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
	static void inner_insert(HtItem<T> *items, size_t cap, std::string key, T *value) noexcept(true) {
		size_t index = ht_hash(key, cap);
		size_t attempts = 0;
		while (items[index].occupied) {
			index = (index + (attempts++ << 1) + 1) & (cap - 1);
		}
		items[index].occupied = true;
		items[index].key = key;
		items[index].value = value;
	}

	// `new_cap` must be a power of 2.
	void reserve_exact(size_t old_cap, size_t new_cap) noexcept(true) {
		HtItem<T> *old_items = this->items;
		HtItem<T> *new_items = new HtItem<T>[new_cap]();
		for (size_t i = 0; i < old_cap; i++) {
			if (old_items[i].occupied) {
				HashTable<T>::inner_insert(new_items, new_cap, std::move(old_items[i].key), old_items[i].value);
			}
		}
		this->items = new_items;
		this->capacity = new_cap;
		delete[] old_items;
	}

public:
	HashTable() noexcept(true) {
		this->capacity = this->len = 0;
		this->items = nullptr;
	}
	// Copy constructor
	HashTable(const HashTable& other) noexcept(std::is_nothrow_copy_constructible<T>::value) {
		this->capacity = other.capacity;
		this->len = other.len;
		this->items = new HtItem<T>[other.capacity]();
		for (size_t i = 0; i < other.capacity; i++) {
			if (other.items[i].occupied) {
				this->items[i].occupied = true;
				this->items[i].key = other.items[i].key;
				this->items[i].value = new T(other.items[i].value);
			}
		}
	}
#if __cplusplus >= 201103L
	// Move constructor
	HashTable(HashTable&& other) noexcept(true) {
		this->capacity = other.capacity;
		this->len = other.len;
		this->items = other.items;
		other.capacity = 0 = other.items = 0;
		other.items = nullptr;
	}
#endif
	~HashTable() noexcept(std::is_nothrow_destructible<T>::value) {
		for (size_t i = 0; i < this->capacity; i++) {
			if (this->items[i].occupied) {
				delete this->items[i].value;
			}
		}
		delete[] this->items;
	}

	// Copy assignment
	HashTable& operator=(const HashTable& other) noexcept(std::is_nothrow_copy_constructible<T>::value) {
		for (size_t i = 0; i < this->capacity; i++) {
			if (this->items[i].occupied) {
				delete this->items[i].value;
			}
		}
		delete[] this->items;

		this->capacity = other.capacity;
		this->len = other.len;
		this->items = new HtItem<T>[other.capacity];
		for (size_t i = 0; i < other.capacity; i++) {
			if (other.items[i].occupied) {
				this->items[i].occupied = true;
				this->items[i].key = other.items[i].key;
				this->items[i].value = new T(other.items[i].value);
			}
		}
	}
#if __cplusplus >= 201103L
	// Move assignment
	HashTable& operator=(HashTable&& other) noexcept(true) {
		this->capacity = other.capacity;
		this->len = other.len;
		this->items = other.items;
		other.capacity = 0;
		other.size = 0;
		other.items = nullptr;
	}
#endif

	bool contains(const std::string& key) const noexcept(true) {
		size_t drop;
		return this->index_of(key, drop);
	}

	void reserve(size_t min_capacity) noexcept(true) {
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
			delete this->items[cur_index].value;
			this->items[cur_index].value = new T(value);
		}
		if (this->capacity == 0) {
			this->reserve_exact(0, INITIAL_CAPACITY);
		}
		size_t cap = this->capacity;
		// resize on 75% capacity
		if (this->len++ << 2 > (cap << 1) + cap) {
			this->reserve_exact(cap, cap << 1);
			return HashTable<T>::inner_insert(this->items, cap << 1, key, new T(value));
		} else {
			return HashTable<T>::inner_insert(this->items, cap, key, new T(value));
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
			return HashTable<T>::inner_insert(this->items, cap << 1, key, new T(value));
		} else {
			return HashTable<T>::inner_insert(this->items, cap, key, new T(value));
		}
	}

	const T& find(const std::string& key) const {
		size_t index;
		if (this->index_of(key, index)) {
			return *this->items[index].value;
		} else {
			throw std::out_of_range("no such key " + key);
		}
	}
	T& find(const std::string& key) {
		size_t index;
		if (this->index_of(key, index)) {
			return *this->items[index].value;
		} else {
			throw std::out_of_range("no such key " + key);
		}
	}
#if __cplusplus >= 201703L
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
#endif

	T& find_or_insert(const std::string& key) noexcept(std::is_nothrow_default_constructible<T>::value) {
		if (this->capacity == 0) {
			this->reserve_exact(0, INITIAL_CAPACITY);
		}
		size_t index;
		if (this->index_of(key, index)) {
			return *this->items[index].value;
		} else {
			this->len++;
			this->items[index].occupied = true;
			this->items[index].key = key;
			return *(this->items[index].value = new T);
		}
	}

	T& operator[](const std::string& key) noexcept(std::is_nothrow_default_constructible<T>::value) {
		return this->find_or_insert(key);
	}

	void remove(const std::string& key) noexcept(std::is_nothrow_destructible<T>::value) {
		size_t index;
		if (this->index_of(key, index)) {
			this->items[index].occupied = false;
			delete this->items[index].value;
			this->len--;
		}
	}

	void clear() noexcept(std::is_nothrow_destructible<T>::value) {
		for (size_t i = 0; i < this->capacity; i++) {
			if (this->items[i].occupied) {
				delete this->items[i].value;
			}
		}
		delete[] this->items;
		this->capacity = this->items = 0;
		this->items = nullptr;
	}

	bool empty() const noexcept(true) {
		return this->len == 0;
	}
	size_t size() const noexcept(true) {
		return this->len;
	}
	size_t max_size() const noexcept(true) {
		return this->capacity;
	}
};

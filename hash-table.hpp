#include <algorithm>
#if __cplusplus >= 202002L
#	include <bit>
#endif
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

// Note that behavior is undefined if there are two keys `a` and `b` such that
// `hash(a) != hash(b) && keyequal(a, b)`. (The inverse of `hash(a) == hash(b)
// && !keyequal(a, b)` is well-defined, since the set of all key items may be
// larger than the set of all `size_t` integers, so hash collisions are expected
// and allowed.) Under these circumstances, an `insert` of one after the other
// may or may not replace the other's contents, and a `find` or similar may
// return either value.
template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
class HashTable {
public:
	using key_type = Key;
	using mapped_type = T;
	using value_type = std::pair<const Key, T>;
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	using hasher = Hash;
	using key_equal = KeyEqual;
	using allocator_type = std::allocator<std::pair<const Key, T>>;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	class iterator;
	class const_iterator;
	using local_iterator = iterator;
	using const_local_iterator = const_iterator;
private:
	using HtItem = std::optional<std::pair<const Key, T>>;

	static const size_t FIB_MULT = 11400714819323198485ull;
	static const size_t HT_PRIME = 151;
	static const size_t INITIAL_CAPACITY = 32;

	size_t capacity;
	size_t len;
	std::unique_ptr<HtItem[]> items;
	Hash hashf;
	KeyEqual cmp;

	using HashNothrow = std::is_nothrow_invocable_r<size_t, Hash, const Key&>;
	using IndexNothrow = std::conjunction<
		HashTable::HashNothrow,
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
	using HashEqualNothrowDefault = std::conjunction<
		std::is_nothrow_default_constructible<Hash>,
		std::is_nothrow_default_constructible<KeyEqual>
	>;
	using HashEqualNothrowCopy = std::conjunction<
		std::is_nothrow_copy_constructible<Hash>,
		std::is_nothrow_copy_constructible<KeyEqual>
	>;
	using HashEqualNothrowMove = std::conjunction<
		std::is_nothrow_move_constructible<Hash>,
		std::is_nothrow_move_constructible<KeyEqual>
	>;
	using HashEqualNothrowDestructible = std::conjunction<
		std::is_nothrow_destructible<Hash>,
		std::is_nothrow_destructible<KeyEqual>
	>;

#if __cplusplus < 202002L
	static inline size_t bit_ceil(size_t x) {
		x--;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		x |= x >> 32;
		x++;
		return x;
	}
#endif

	static size_t hash(const Key& val, size_t cap, const Hash &hashf) noexcept(HashNothrow::value) {
#if __cplusplus >= 202002L
		int shift = std::countl_zero(cap) + 1;
#else
		int shift = __builtin_clzll(cap) + 1;
#endif
		size_t hash = hashf(val);
		return (hash * HashTable::FIB_MULT) >> shift;
	}

	std::pair<bool, size_t> index_of(const Key& key) const noexcept(IndexNothrow::value) {
		size_t cap = this->capacity;
		if (cap == 0) {
			return std::make_pair(false, 0);
		}
		size_t index = HashTable::hash(key, cap, this->hashf);
		size_t attempts = 0;
		while (this->items[index]) {
			if (this->cmp((*this->items[index]).first, key)) {
				return std::make_pair(true, index);
			}
			index = (index + 1) & (cap - 1);
			if (attempts > this->capacity) {
				return std::make_pair(false, index);
			}
		}
		return std::make_pair(false, index);
	}

	// Assumes `key` does not already exist in `items`, so it continues to
	// hash as long as that slot is occupied.
	static std::pair<iterator, bool> inner_insert(std::unique_ptr<HtItem[]> &items, size_t cap, value_type pair, Hash &hashf) noexcept(HashNothrow::value) {
		size_t index = HashTable::hash(pair.first, cap, hashf);
		while (items[index]) {
			index = (index + 1) & (cap - 1);
		}
		items[index].emplace(std::move(pair));
		return std::make_pair(iterator(items.get() + index), true);
	}

	template<class... Args>
	std::pair<iterator, bool> emplace_unique_hint(iterator hint, Args&&... args) noexcept(IndexNothrow::value && ItemNothrowMove::value && std::is_nothrow_invocable<value_type, Args...>::value) {
		value_type pair(std::forward<Args>(args)...);
		if (hint != this->end() && (!hint.item->has_value() || this->cmp((*hint).first, pair.first))) {
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
			return HashTable::inner_insert(this->items, cap << 1, std::move(pair), this->hashf);
		} else {
			return HashTable::inner_insert(this->items, cap, std::move(pair), this->hashf);
		}
	}

	// `new_cap` must be a power of 2.
	void reserve_exact(size_t old_cap, size_t new_cap) {
		std::unique_ptr<HtItem[]> new_items(new HtItem[new_cap]);
		for (size_t i = 0; i < old_cap; i++) {
			if (this->items[i]) {
				HashTable::inner_insert(
					new_items,
					new_cap,
					std::move(*this->items[i]),
					this->hashf
				);
			}
		}
		this->items = std::move(new_items);
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

	HashTable() noexcept(HashEqualNothrowDefault::value) {
		this->capacity = this->len = 0;
		this->items = nullptr;
		this->hashf = Hash{};
		this->cmp = KeyEqual{};
	}
	HashTable(size_t bucket_count, const Hash& hash = Hash{}, const KeyEqual& cmp = KeyEqual{}, const allocator_type& alloc = allocator_type{}) {
		if (bucket_count == 0) {
			this->capacity = 0;
			this->items = nullptr;
		} else {
			// capacity must always be a power of 2
#if __cplusplus >= 202002L
			this->capacity = std::bit_ceil(bucket_count);
#else
			this->capacity = HashTable::bit_ceil(bucket_count);
#endif
			this->items = std::make_unique<HtItem[]>(this->capacity);
		}
		this->len = 0;
		this->hashf = Hash{hash};
		this->cmp = KeyEqual{cmp};
	}
	HashTable(std::initializer_list<value_type> init, size_t bucket_count = 0, const Hash& hash = Hash{}, const KeyEqual& cmp = KeyEqual{}, const allocator_type& alloc = allocator_type{}) {
		// capacity must always be a power of 2
#if __cplusplus >= 202002L
		this->capacity = std::bit_ceil(std::max(bucket_count, init.size()));
#else
		this->capacity = HashTable::bit_ceil(std::max(bucket_count, init.size()));
#endif
		this->len = 0;
		this->items = std::make_unique<HtItem[]>(this->capacity);
		this->hashf = Hash{hash};
		this->cmp = KeyEqual{cmp};
		for (auto item : std::move(init)) {
			auto [contains, index] = this->index_of(item.first);
			if (contains && this->cmp((*this->items[index]).first, item.first)) {
				this->items[index].emplace(std::move(item));
			} else if (!this->items[index].has_value()) {
				this->len++;
				this->items[index].emplace(std::move(item));
			} else {
				this->len++;
				HashTable::inner_insert(this->items, this->capacity, std::move(item), this->hashf);
			}
		}
	}
	// Copy constructor
	HashTable(const HashTable& other) {
		if (other.capacity == 0) {
			this->capacity = this->len = 0;
			this->items = nullptr;
			return;
		}
		this->capacity = other.capacity;
		this->len = other.len;
		this->items = std::make_unique<HtItem[]>(other.capacity);
		this->hashf = Hash{other.hashf};
		this->cmp = KeyEqual{other.cmp};
		for (size_t i = 0; i < other.capacity; i++) {
			if (other.items[i]) {
				this->items[i].emplace((*other.items[i]).first, (*other.items[i]).second);
			}
		}
	}
	// Move constructor
	HashTable(HashTable&& other) noexcept(HashEqualNothrowMove::value) {
		this->capacity = other.capacity;
		this->len = other.len;
		this->items = std::move(other.items);
		this->hashf = std::move(other.hashf);
		this->cmp = std::move(other.cmp);
		other.capacity = other.len = 0;
		other.items = nullptr;
	}
	~HashTable() noexcept(ItemNothrowDestructible::value && HashEqualNothrowDestructible::value) = default;

	// Copy assignment
	HashTable& operator=(const HashTable& other) {
		if (this == &other) {
			return *this;
		}
		std::unique_ptr<HtItem[]> new_items(new HtItem[other.capacity]);
		for (size_t i = 0; i < other.capacity; i++) {
			if (other.items[i]) {
				new_items[i].emplace((*other.items[i]).first, (*other.items[i]).second);
			}
		}
		this->capacity = other.capacity;
		this->len = other.len;
		this->items = std::move(new_items);
		this->hashf = Hash{other.hashf};
		this->cmp = KeyEqual{other.cmp};
		return *this;
	}
	// Move assignment
	HashTable& operator=(HashTable&& other) noexcept(ItemNothrowDestructible::value && HashEqualNothrowMove::value && HashEqualNothrowDestructible::value) {
		if (this == &other) {
			return *this;
		}
		this->capacity = other.capacity;
		this->len = other.len;
		this->items = std::move(other.items);
		this->hashf = std::move(other.hashf);
		this->cmp = std::move(other.cmp);
		other.capacity = other.len = 0;
		other.items = nullptr;
		return *this;
	}
	// assign from initializer list
	HashTable& operator=(std::initializer_list<value_type> ilist) {
		// to keep it below the 75% threshold
		if (ilist.size() + (ilist.size() >> 1) > this->capacity) {
			// capacity must always be a power of 2
#if __cplusplus >= 202002L
			size_t new_cap = std::bit_ceil(ilist.size() + (ilist.size() >> 1));
#else
			size_t new_cap = HashTable::bit_ceil(ilist.size() + (ilist.size() >> 1));
#endif
			this->reserve_exact(this->capacity, new_cap);
		}
		this->len = 0;
		this->items = std::make_unique<HtItem[]>(this->capacity);
		for (auto item : std::move(ilist)) {
			auto [contains, index] = this->index_of(item.first);
			if (contains && this->cmp((*this->items[index]).first, item.first)) {
				this->items[index].emplace(std::move(item));
			} else if (!this->items[index].has_value()) {
				this->len++;
				this->items[index].emplace(std::move(item));
			} else {
				this->len++;
				HashTable::inner_insert(this->items, this->capacity, std::move(item), this->hashf);
			}
		}
		return *this;
	}

	allocator_type get_allocator() const noexcept {
		return allocator_type{};
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
	void reserve(size_t min_capacity) {
		size_t old_cap = this->capacity;
		if (min_capacity <= old_cap) {
			return;
		}

		// capacity must always be a power of 2
#if __cplusplus >= 202002L
		size_t new_cap = std::bit_ceil(min_capacity);
#else
		size_t new_cap = HashTable::bit_ceil(min_capacity);
#endif
		this->reserve_exact(old_cap, new_cap);
	}

	void shrink_to_fit() {
		if (this->len == 0) {
			this->clear();
			return;
		}
		size_t old_cap = this->capacity;
		// capacity must always be a power of 2
#if __cplusplus >= 202002L
		size_t new_cap = std::bit_ceil(capacity);
#else
		size_t new_cap = HashTable::bit_ceil(capacity);
#endif
		if (new_cap >= old_cap) {
			return;
		}
		this->reserve_exact(old_cap, new_cap);
	}

	void rehash(size_t min_capacity) {
		size_t old_cap = this->capacity;
		if (min_capacity > old_cap) {
			return this->reserve(min_capacity);
		} else {
			// to force a rehashing without changing capacity
			this->reserve_exact(old_cap, old_cap);
		}
	}

	std::pair<iterator, bool> insert(const value_type& value) {
		return this->emplace(value_type{value});
	}
	std::pair<iterator, bool> insert(value_type&& value) {
		return this->emplace(std::move(value));
	}
	void insert(std::initializer_list<value_type> ilist) {
		// 1.5x size due to the 75% capacity limit (to avoid potentially
		// double-reserving).
#if __cplusplus >= 202002L
		size_t list_size_rounded = std::bit_ceil(ilist.size() + (ilist.size() >> 1));
#else
		size_t list_size_rounded = HashTable::bit_ceil(ilist.size() + (ilist.size() >> 1));
#endif
		if (this->capacity == 0) {
			this->reserve_exact(0, std::max(HashTable::INITIAL_CAPACITY, list_size_rounded));
		}
		size_t cap = this->capacity;
		// resize on 75% capacity
		if ((this->len + ilist.size()) << 2 > (cap << 1) + cap) {
			this->reserve_exact(cap, std::max(cap << 1, list_size_rounded));
		}
		for (auto item : std::move(ilist)) {
			auto [contains, index] = this->index_of(item.first);
			if (contains && this->cmp((*this->items[index]).first, item.first)) {
				this->items[index].emplace(std::move(item));
			} else if (!this->items[index].has_value()) {
				this->len++;
				this->items[index].emplace(std::move(item));
			} else {
				this->len++;
				HashTable::inner_insert(this->items, cap, std::move(item), this->hashf);
			}
		}
	}
	std::pair<iterator, bool> insert_or_assign(const Key& key, const T& value) {
		return this->emplace_or_assign(Key{key}, T{value});
	}
	std::pair<iterator, bool> insert_or_assign(Key&& key, T&& value) {
		return this->emplace_or_assign(std::move(key), std::move(value));
	}
	std::pair<iterator, bool> insert_unique(const Key& key, const T& value) {
		return this->insert_unique(Key{key}, T{value});
	}
	std::pair<iterator, bool> insert_unique(Key&& key, T&& value) {
		if (this->capacity == 0) {
			this->reserve_exact(0, HashTable::INITIAL_CAPACITY);
		}
		value_type pair(std::move(key), std::move(value));
		size_t cap = this->capacity;
		// resize on 75% capacity
		if (this->len++ << 2 > (cap << 1) + cap) {
			this->reserve_exact(cap, cap << 1);
			return HashTable::inner_insert(this->items, cap << 1, std::move(pair), this->hashf);
		} else {
			return HashTable::inner_insert(this->items, cap, std::move(pair), this->hashf);
		}
	}

	template<class... Args>
	std::pair<iterator, bool> emplace(Args&&... args) {
		value_type pair(std::forward<Args>(args)...);
		auto [contains, cur_index] = this->index_of(pair.first);
		if (contains) {
			return std::make_pair(iterator(this->items.get() + cur_index), false);
		}
		return this->emplace_unique_hint(iterator(this->items.get() + cur_index), std::move(pair));
	}

	template<class... Args>
	iterator emplace_hint(const_iterator hint, Args&&... args) {
		value_type pair(std::forward<Args>(args)...);
		if (hint != this->cend()) {
			HtItem* item = hint.item;
			bool had_value = item->has_value();
			if (!had_value) {
				this->len++;
				item->emplace(std::move(pair));
			}
			return iterator(item);
		}
		// hint was bad, ignore it
		return this->emplace(std::move(pair)).first;
	}

	template<class... Args>
	std::pair<iterator, bool> emplace_or_assign(Args&&... args) {
		value_type pair(std::forward<Args>(args)...);
		auto [contains, cur_index] = this->index_of(pair.first);
		if (contains) {
			this->items[cur_index].emplace(std::move(pair));
			return std::make_pair(iterator(this->items.get() + cur_index), false);
		}
		return this->emplace_unique_hint(iterator(this->items.get() + cur_index), std::move(pair));
	}

	iterator find(const Key& key) noexcept(IndexNothrow::value) {
		auto [contains, index] = this->index_of(key);
		if (contains) {
			return iterator(this->items.get() + index);
		} else {
			return this->end();
		}
	}
	const_iterator find(const Key& key) const noexcept(IndexNothrow::value) {
		auto [contains, index] = this->index_of(key);
		if (contains) {
			return const_iterator(this->items.get() + index);
		} else {
			return this->cend();
		}
	}
	T& find_or_insert(const Key& key) {
		return this->find_or_insert(Key{key});
	}
	T& find_or_insert(Key&& key) {
		if (this->capacity == 0) {
			this->reserve_exact(0, HashTable::INITIAL_CAPACITY);
		}
		auto [contains, index] = this->index_of(key);
		if (contains) {
			return (*this->items[index]).second;
		} else {
			this->len++;
			this->items[index].emplace(std::move(key), T{});
			return (*this->items[index]).second;
		}
	}

	T& operator[](const Key& key) {
		return this->find_or_insert(key);
	}
	T& operator[](Key&& key) {
		return this->find_or_insert(std::move(key));
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

	std::pair<iterator, iterator> equal_range(const Key& key) {
		auto [contains, index] = this->index_of(key);
		if (contains) {
			iterator out(this->items.get() + index);
			return std::make_pair(out, out);
		} else {
			return std::make_pair(this->end(), this->end());
		}
	}
	std::pair<const_iterator, const_iterator> equal_range(const Key& key) const {
		auto [contains, index] = this->index_of(key);
		if (contains) {
			const_iterator out(this->items.get() + index);
			return std::make_pair(out, out);
		} else {
			return std::make_pair(this->cend(), this->cend());
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
		this->capacity = this->len = 0;
		this->items = nullptr;
	}

	void swap(HashTable<Key, T>& other) noexcept(HashEqualNothrowMove::value) {
		auto items = std::move(this->items);
		auto capacity = this->capacity;
		auto len = this->len;
		auto hashf = std::move(this->hashf);
		auto cmp = std::move(this->cmp);

		this->items = std::move(other.items);
		this->capacity = other.capacity;
		this->len = other.len;
		this->hashf = std::move(other.hashf);
		this->cmp = std::move(other.cmp);

		other.items = std::move(items);
		other.capacity = capacity;
		other.len = len;
		other.hashf = std::move(hashf);
		other.cmp = std::move(cmp);
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

	iterator begin() noexcept {
		return iterator(this->items.get(), this->items.get() + this->capacity);
	}
	iterator end() noexcept {
		return iterator(this->items.get() + this->capacity);
	}
	const_iterator begin() const noexcept {
		return const_iterator(this->items.get(), this->items.get() + this->capacity);
	}
	const_iterator end() const noexcept {
		return const_iterator(this->items.get() + this->capacity);
	}
	const_iterator cbegin() const noexcept {
		return const_iterator(this->items.get(), this->items.get() + this->capacity);
	}
	const_iterator cend() const noexcept {
		return const_iterator(this->items.get() + this->capacity);
	}

	local_iterator begin(size_t n) noexcept {
		if (n >= this->len) {
			return this->end();
		} else {
			return iterator(this->items.get() + n);
		}
	}
	local_iterator end(size_t n) noexcept {
		return this->begin(n);
	}
	const_local_iterator begin(size_t n) const noexcept {
		return this->cbegin(n);
	}
	const_local_iterator end(size_t n) const noexcept {
		return this->cbegin(n);
	}
	const_local_iterator cbegin(size_t n) const noexcept {
		if (n >= this->len) {
			return this->cend();
		} else {
			return const_iterator(this->items.get() + n);
		}
	}
	const_local_iterator cend(size_t n) const noexcept {
		return this->cbegin(n);
	}

	size_t bucket_count() const noexcept {
		return this->capacity;
	}
	size_t max_bucket_count() const noexcept {
		return (size_t) -1;
	}
	float load_factor() const noexcept {
		return (float) this->len / (float) this->capacity;
	}
	float max_load_factor() const noexcept {
		return 1.0;
	}
	void max_load_factor(float ml) noexcept {
		(void) ml;
	}

	Hash hash_function() const noexcept(std::is_nothrow_copy_constructible<Hash>::value) {
		return this->hashf;
	}
	KeyEqual key_eq() const noexcept(std::is_nothrow_copy_constructible<KeyEqual>::value) {
		return this->cmp;
	}
};

namespace std {
	template<class Key, class T>
	void swap(HashTable<Key, T>& h1, HashTable<Key, T>& h2) noexcept(noexcept(h1.swap(h2))) {
		h1.swap(h2);
	}

	template<class Key, class T, class Pred>
	size_t erase_if(HashTable<Key, T>& c, Pred pred) {
		auto old_size = c.size();
		for (auto i = c.begin(), last = c.end(); i != last;) {
			if (pred(*i)) {
				i = c.erase(i);
			} else {
				++i;
			}
		}
		return old_size - c.size();
	}
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

// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <deque>
#include <algorithm>
#include "base/optional.h"

namespace base {

using std::begin;
using std::end;

template <
	typename Key,
	typename Type,
	typename Compare = std::less<>>
class flat_map;

template <
	typename Key,
	typename Type,
	typename Compare = std::less<>>
class flat_multi_map;

template <
	typename Me,
	typename Key,
	typename Type,
	typename iterator_impl,
	typename pointer_impl,
	typename reference_impl>
class flat_multi_map_iterator_base_impl;

template <typename Key, typename Value>
struct flat_multi_map_pair_type {
	using first_type = const Key;
	using second_type = Value;

	constexpr flat_multi_map_pair_type()
	: first()
	, second() {
	}

	template <typename OtherKey, typename OtherValue>
	constexpr flat_multi_map_pair_type(OtherKey &&key, OtherValue &&value)
	: first(std::forward<OtherKey>(key))
	, second(std::forward<OtherValue>(value)) {
	}

	flat_multi_map_pair_type(const flat_multi_map_pair_type &pair)
	: first(pair.first)
	, second(pair.second) {
	}

	flat_multi_map_pair_type(flat_multi_map_pair_type &&pair)
	: first(std::move(const_cast<Key&>(pair.first)))
	, second(std::move(pair.second)) {
	}

	flat_multi_map_pair_type &operator=(const flat_multi_map_pair_type&) = delete;
	flat_multi_map_pair_type &operator=(flat_multi_map_pair_type &&other) {
		const_cast<Key&>(first) = std::move(const_cast<Key&>(other.first));
		second = std::move(other.second);
		return *this;
	}

	void swap(flat_multi_map_pair_type &other) {
		using std::swap;

		if (this != &other) {
			std::swap(
				const_cast<Key&>(first),
				const_cast<Key&>(other.first));
			std::swap(second, other.second);
		}
	}

	const Key first;
	Value second;

};

template <
	typename Me,
	typename Key,
	typename Type,
	typename iterator_impl,
	typename pointer_impl,
	typename reference_impl>
class flat_multi_map_iterator_base_impl {
public:
	using iterator_category = typename iterator_impl::iterator_category;

	using pair_type = flat_multi_map_pair_type<Key, Type>;
	using value_type = pair_type;
	using difference_type = typename iterator_impl::difference_type;
	using pointer = pointer_impl;
	using reference = reference_impl;

	flat_multi_map_iterator_base_impl(iterator_impl impl = iterator_impl())
		: _impl(impl) {
	}

	reference operator*() const {
		return *_impl;
	}
	pointer operator->() const {
		return std::addressof(**this);
	}
	Me &operator++() {
		++_impl;
		return static_cast<Me&>(*this);
	}
	Me operator++(int) {
		return _impl++;
	}
	Me &operator--() {
		--_impl;
		return static_cast<Me&>(*this);
	}
	Me operator--(int) {
		return _impl--;
	}
	Me &operator+=(difference_type offset) {
		_impl += offset;
		return static_cast<Me&>(*this);
	}
	Me operator+(difference_type offset) const {
		return _impl + offset;
	}
	Me &operator-=(difference_type offset) {
		_impl -= offset;
		return static_cast<Me&>(*this);
	}
	Me operator-(difference_type offset) const {
		return _impl - offset;
	}
	template <
		typename other_me,
		typename other_iterator_impl,
		typename other_pointer_impl,
		typename other_reference_impl>
	difference_type operator-(
			const flat_multi_map_iterator_base_impl<
				other_me,
				Key,
				Type,
				other_iterator_impl,
				other_pointer_impl,
				other_reference_impl> &right) const {
		return _impl - right._impl;
	}
	reference operator[](difference_type offset) const {
		return _impl[offset];
	}

	template <
		typename other_me,
		typename other_iterator_impl,
		typename other_pointer_impl,
		typename other_reference_impl>
	bool operator==(
			const flat_multi_map_iterator_base_impl<
				other_me,
				Key,
				Type,
				other_iterator_impl,
				other_pointer_impl,
				other_reference_impl> &right) const {
		return _impl == right._impl;
	}
	template <
		typename other_me,
		typename other_iterator_impl,
		typename other_pointer_impl,
		typename other_reference_impl>
	bool operator!=(
			const flat_multi_map_iterator_base_impl<
				other_me,
				Key,
				Type,
				other_iterator_impl,
				other_pointer_impl,
				other_reference_impl> &right) const {
		return _impl != right._impl;
	}
	template <
		typename other_me,
		typename other_iterator_impl,
		typename other_pointer_impl,
		typename other_reference_impl>
	bool operator<(
			const flat_multi_map_iterator_base_impl<
				other_me,
				Key,
				Type,
				other_iterator_impl,
				other_pointer_impl,
				other_reference_impl> &right) const {
		return _impl < right._impl;
	}

private:
	iterator_impl _impl;

	template <
		typename OtherKey,
		typename OtherType,
		typename OtherCompare>
	friend class flat_multi_map;

	template <
		typename OtherMe,
		typename OtherKey,
		typename OtherType,
		typename other_iterator_impl,
		typename other_pointer_impl,
		typename other_reference_impl>
	friend class flat_multi_map_iterator_base_impl;

};

template <typename Key, typename Type, typename Compare>
class flat_multi_map {
public:
	class iterator;
	class const_iterator;
	class reverse_iterator;
	class const_reverse_iterator;

private:
	using pair_type = flat_multi_map_pair_type<Key, Type>;
	using impl_t = std::deque<pair_type>;

	using iterator_base = flat_multi_map_iterator_base_impl<
		iterator,
		Key,
		Type,
		typename impl_t::iterator,
		pair_type*,
		pair_type&>;
	using const_iterator_base = flat_multi_map_iterator_base_impl<
		const_iterator,
		Key,
		Type,
		typename impl_t::const_iterator,
		const pair_type*,
		const pair_type&>;
	using reverse_iterator_base = flat_multi_map_iterator_base_impl<
		reverse_iterator,
		Key,
		Type,
		typename impl_t::reverse_iterator,
		pair_type*,
		pair_type&>;
	using const_reverse_iterator_base = flat_multi_map_iterator_base_impl<
		const_reverse_iterator,
		Key,
		Type,
		typename impl_t::const_reverse_iterator,
		const pair_type*,
		const pair_type&>;

public:
	using value_type = pair_type;
	using size_type = typename impl_t::size_type;
	using difference_type = typename impl_t::difference_type;
	using pointer = pair_type*;
	using const_pointer = const pair_type*;
	using reference = pair_type&;
	using const_reference = const pair_type&;

	class iterator : public iterator_base {
	public:
		using iterator_base::iterator_base;
		iterator() = default;
		iterator(const iterator_base &other) : iterator_base(other) {
		}
		friend class const_iterator;

	};
	class const_iterator : public const_iterator_base {
	public:
		using const_iterator_base::const_iterator_base;
		const_iterator() = default;
		const_iterator(const_iterator_base other) : const_iterator_base(other) {
		}
		const_iterator(const iterator &other) : const_iterator_base(other._impl) {
		}

	};
	class reverse_iterator : public reverse_iterator_base {
	public:
		using reverse_iterator_base::reverse_iterator_base;
		reverse_iterator() = default;
		reverse_iterator(reverse_iterator_base other) : reverse_iterator_base(other) {
		}
		friend class const_reverse_iterator;

	};
	class const_reverse_iterator : public const_reverse_iterator_base {
	public:
		using const_reverse_iterator_base::const_reverse_iterator_base;
		const_reverse_iterator() = default;
		const_reverse_iterator(const_reverse_iterator_base other) : const_reverse_iterator_base(other) {
		}
		const_reverse_iterator(const reverse_iterator &other) : const_reverse_iterator_base(other._impl) {
		}

	};

	flat_multi_map() = default;
	flat_multi_map(const flat_multi_map &other) = default;
	flat_multi_map(flat_multi_map &&other) = default;
	flat_multi_map &operator=(const flat_multi_map &other) {
		auto copy = other;
		return (*this = std::move(copy));
	}
	flat_multi_map &operator=(flat_multi_map &&other) = default;

	template <
		typename Iterator,
		typename = typename std::iterator_traits<Iterator>::iterator_category>
	flat_multi_map(Iterator first, Iterator last)
	: _data(first, last) {
		std::sort(std::begin(impl()), std::end(impl()), compare());
	}

	flat_multi_map(std::initializer_list<pair_type> iter)
	: flat_multi_map(iter.begin(), iter.end()) {
	}

	size_type size() const {
		return impl().size();
	}
	bool empty() const {
		return impl().empty();
	}
	void clear() {
		impl().clear();
	}

	iterator begin() {
		return impl().begin();
	}
	iterator end() {
		return impl().end();
	}
	const_iterator begin() const {
		return impl().begin();
	}
	const_iterator end() const {
		return impl().end();
	}
	const_iterator cbegin() const {
		return impl().cbegin();
	}
	const_iterator cend() const {
		return impl().cend();
	}
	reverse_iterator rbegin() {
		return impl().rbegin();
	}
	reverse_iterator rend() {
		return impl().rend();
	}
	const_reverse_iterator rbegin() const {
		return impl().rbegin();
	}
	const_reverse_iterator rend() const {
		return impl().rend();
	}
	const_reverse_iterator crbegin() const {
		return impl().crbegin();
	}
	const_reverse_iterator crend() const {
		return impl().crend();
	}

	reference front() {
		return *begin();
	}
	const_reference front() const {
		return *begin();
	}
	reference back() {
		return *(end() - 1);
	}
	const_reference back() const {
		return *(end() - 1);
	}

	iterator insert(const value_type &value) {
		if (empty() || compare()(value.first, front().first)) {
			impl().push_front(value);
			return begin();
		} else if (!compare()(value.first, back().first)) {
			impl().push_back(value);
			return (end() - 1);
		}
		auto where = getUpperBound(value.first);
		return impl().insert(where, value);
	}
	iterator insert(value_type &&value) {
		if (empty() || compare()(value.first, front().first)) {
			impl().push_front(std::move(value));
			return begin();
		} else if (!compare()(value.first, back().first)) {
			impl().push_back(std::move(value));
			return (end() - 1);
		}
		auto where = getUpperBound(value.first);
		return impl().insert(where, std::move(value));
	}
	template <typename... Args>
	iterator emplace(Args&&... args) {
		return insert(value_type(std::forward<Args>(args)...));
	}

	bool removeOne(const Key &key) {
		if (empty()
			|| compare()(key, front().first)
			|| compare()(back().first, key)) {
			return false;
		}
		auto where = getLowerBound(key);
		if (compare()(key, where->first)) {
			return false;
		}
		impl().erase(where);
		return true;
	}
	int removeAll(const Key &key) {
		if (empty()
			|| compare()(key, front().first)
			|| compare()(back().first, key)) {
			return 0;
		}
		auto range = getEqualRange(key);
		if (range.first == range.second) {
			return 0;
		}
		const auto result = (range.second - range.first);
		impl().erase(range.first, range.second);
		return result;
	}

	iterator erase(const_iterator where) {
		return impl().erase(where._impl);
	}
	iterator erase(const_iterator from, const_iterator till) {
		return impl().erase(from._impl, till._impl);
	}
	int erase(const Key &key) {
		return removeAll(key);
	}

	iterator findFirst(const Key &key) {
		if (empty()
			|| compare()(key, front().first)
			|| compare()(back().first, key)) {
			return end();
		}
		auto where = getLowerBound(key);
		return compare()(key, where->first) ? impl().end() : where;
	}

	const_iterator findFirst(const Key &key) const {
		if (empty()
			|| compare()(key, front().first)
			|| compare()(back().first, key)) {
			return end();
		}
		auto where = getLowerBound(key);
		return compare()(key, where->first) ? impl().end() : where;
	}

	template <typename OtherKey>
	iterator findFirst(const OtherKey &key) {
		if (empty()
			|| compare()(key, front().first)
			|| compare()(back().first, key)) {
			return end();
		}
		auto where = getLowerBound(key);
		return compare()(key, where->first) ? impl().end() : where;
	}

	template <typename OtherKey>
	const_iterator findFirst(const OtherKey &key) const {
		if (empty()
			|| compare()(key, front().first)
			|| compare()(back().first, key)) {
			return end();
		}
		auto where = getLowerBound(key);
		return compare()(key, where->first) ? impl().end() : where;
	}

	bool contains(const Key &key) const {
		return findFirst(key) != end();
	}
	int count(const Key &key) const {
		if (empty()
			|| compare()(key, front().first)
			|| compare()(back().first, key)) {
			return 0;
		}
		auto range = getEqualRange(key);
		return (range.second - range.first);
	}

private:
	friend class flat_map<Key, Type, Compare>;

	struct transparent_compare : Compare {
		inline constexpr const Compare &initial() const noexcept {
			return *this;
		}

		template <
			typename OtherType1,
			typename OtherType2,
			typename = std::enable_if_t<
				!std::is_same_v<std::decay_t<OtherType1>, pair_type> &&
				!std::is_same_v<std::decay_t<OtherType2>, pair_type>>>
		inline constexpr auto operator()(
				OtherType1 &&a,
				OtherType2 &&b) const {
			return initial()(
				std::forward<OtherType1>(a),
				std::forward<OtherType2>(b));
		}
		template <
			typename OtherType1,
			typename OtherType2>
		inline constexpr auto operator()(
				OtherType1 &&a,
				OtherType2 &&b) const -> std::enable_if_t<
		std::is_same_v<std::decay_t<OtherType1>, pair_type> &&
		std::is_same_v<std::decay_t<OtherType2>, pair_type>, bool> {
			return initial()(a.first, b.first);
		}
		template <
			typename OtherType,
			typename = std::enable_if_t<
				!std::is_same_v<std::decay_t<OtherType>, pair_type>>>
		inline constexpr auto operator()(
				const pair_type &a,
				OtherType &&b) const {
			return operator()(a.first, std::forward<OtherType>(b));
		}
		template <
			typename OtherType,
			typename = std::enable_if_t<
				!std::is_same_v<std::decay_t<OtherType>, pair_type>>>
		inline constexpr auto operator()(
				OtherType &&a,
				const pair_type &b) const {
			return operator()(std::forward<OtherType>(a), b.first);
		}

	};
	struct Data : transparent_compare {
		template <typename ...Args>
		Data(Args &&...args)
		: elements(std::forward<Args>(args)...) {
		}

		impl_t elements;
	};

	Data _data;
	const transparent_compare &compare() const noexcept {
		return _data;
	}
	const impl_t &impl() const noexcept {
		return _data.elements;
	}
	impl_t &impl() noexcept {
		return _data.elements;
	}

	template <typename OtherKey>
	typename impl_t::iterator getLowerBound(const OtherKey &key) {
		return std::lower_bound(
			std::begin(impl()),
			std::end(impl()),
			key,
			compare());
	}
	template <typename OtherKey>
	typename impl_t::const_iterator getLowerBound(const OtherKey &key) const {
		return std::lower_bound(
			std::begin(impl()),
			std::end(impl()),
			key,
			compare());
	}
	template <typename OtherKey>
	typename impl_t::iterator getUpperBound(const OtherKey &key) {
		return std::upper_bound(
			std::begin(impl()),
			std::end(impl()),
			key,
			compare());
	}
	template <typename OtherKey>
	typename impl_t::const_iterator getUpperBound(const OtherKey &key) const {
		return std::upper_bound(
			std::begin(impl()),
			std::end(impl()),
			key,
			compare());
	}
	template <typename OtherKey>
	std::pair<
		typename impl_t::iterator,
		typename impl_t::iterator
	> getEqualRange(const OtherKey &key) {
		return std::equal_range(
			std::begin(impl()),
			std::end(impl()),
			key,
			compare());
	}
	template <typename OtherKey>
	std::pair<
		typename impl_t::const_iterator,
		typename impl_t::const_iterator
	> getEqualRange(const OtherKey &key) const {
		return std::equal_range(
			std::begin(impl()),
			std::end(impl()),
			key,
			compare());
	}

};

template <typename Key, typename Type, typename Compare>
class flat_map : private flat_multi_map<Key, Type, Compare> {
	using parent = flat_multi_map<Key, Type, Compare>;
	using pair_type = typename parent::pair_type;

public:
	using value_type = typename parent::value_type;
	using size_type = typename parent::size_type;
	using difference_type = typename parent::difference_type;
	using pointer = typename parent::pointer;
	using const_pointer = typename parent::const_pointer;
	using reference = typename parent::reference;
	using const_reference = typename parent::const_reference;
	using iterator = typename parent::iterator;
	using const_iterator = typename parent::const_iterator;
	using reverse_iterator = typename parent::reverse_iterator;
	using const_reverse_iterator = typename parent::const_reverse_iterator;

	flat_map() = default;

	template <
		typename Iterator,
		typename = typename std::iterator_traits<Iterator>::iterator_category
	>
	flat_map(Iterator first, Iterator last) : parent(first, last) {
		finalize();
	}

	flat_map(std::initializer_list<pair_type> iter) : parent(iter.begin(), iter.end()) {
		finalize();
	}

	using parent::parent;
	using parent::size;
	using parent::empty;
	using parent::clear;
	using parent::begin;
	using parent::end;
	using parent::cbegin;
	using parent::cend;
	using parent::rbegin;
	using parent::rend;
	using parent::crbegin;
	using parent::crend;
	using parent::front;
	using parent::back;
	using parent::erase;
	using parent::contains;

	std::pair<iterator, bool> insert(const value_type &value) {
		if (this->empty() || this->compare()(value.first, this->front().first)) {
			this->impl().push_front(value);
			return { this->begin(), true };
		} else if (this->compare()(this->back().first, value.first)) {
			this->impl().push_back(value);
			return { this->end() - 1, true };
		}
		auto where = this->getLowerBound(value.first);
		if (this->compare()(value.first, where->first)) {
			return { this->impl().insert(where, value), true };
		}
		return { where, false };
	}
	std::pair<iterator, bool> insert(value_type &&value) {
		if (this->empty() || this->compare()(value.first, this->front().first)) {
			this->impl().push_front(std::move(value));
			return { this->begin(), true };
		} else if (this->compare()(this->back().first, value.first)) {
			this->impl().push_back(std::move(value));
			return { this->end() - 1, true };
		}
		auto where = this->getLowerBound(value.first);
		if (this->compare()(value.first, where->first)) {
			return { this->impl().insert(where, std::move(value)), true };
		}
		return { where, false };
	}
	std::pair<iterator, bool> insert_or_assign(
			const Key &key,
			const Type &value) {
		if (this->empty() || this->compare()(key, this->front().first)) {
			this->impl().emplace_front(key, value);
			return { this->begin(), true };
		} else if (this->compare()(this->back().first, key)) {
			this->impl().emplace_back(key, value);
			return { this->end() - 1, true };
		}
		auto where = this->getLowerBound(key);
		if (this->compare()(key, where->first)) {
			return { this->impl().insert(where, value_type(key, value)), true };
		}
		where->second = value;
		return { where, false };
	}
	std::pair<iterator, bool> insert_or_assign(
			const Key &key,
			Type &&value) {
		if (this->empty() || this->compare()(key, this->front().first)) {
			this->impl().emplace_front(key, std::move(value));
			return { this->begin(), true };
		} else if (this->compare()(this->back().first, key)) {
			this->impl().emplace_back(key, std::move(value));
			return { this->end() - 1, true };
		}
		auto where = this->getLowerBound(key);
		if (this->compare()(key, where->first)) {
			return { this->impl().insert(where, value_type(key, std::move(value))), true };
		}
		where->second = std::move(value);
		return { where, false };
	}
	template <typename OtherKey, typename... Args>
	std::pair<iterator, bool> emplace(
			OtherKey &&key,
			Args&&... args) {
		return this->insert(value_type(
			std::forward<OtherKey>(key),
			Type(std::forward<Args>(args)...)));
	}
	template <typename... Args>
	std::pair<iterator, bool> emplace_or_assign(
			const Key &key,
			Args&&... args) {
		return this->insert_or_assign(
			key,
			Type(std::forward<Args>(args)...));
	}
	template <typename... Args>
	std::pair<iterator, bool> try_emplace(
			const Key &key,
			Args&&... args) {
		if (this->empty() || this->compare()(key, this->front().first)) {
			this->impl().push_front(value_type(
				key,
				Type(std::forward<Args>(args)...)));
			return { this->begin(), true };
		} else if (this->compare()(this->back().first, key)) {
			this->impl().push_back(value_type(
				key,
				Type(std::forward<Args>(args)...)));
			return { this->end() - 1, true };
		}
		auto where = this->getLowerBound(key);
		if (this->compare()(key, where->first)) {
			return {
				this->impl().insert(
					where,
					value_type(
						key,
						Type(std::forward<Args>(args)...))),
				true
			};
		}
		return { where, false };
	}

	bool remove(const Key &key) {
		return this->removeOne(key);
	}

	iterator find(const Key &key) {
		return this->findFirst(key);
	}
	const_iterator find(const Key &key) const {
		return this->findFirst(key);
	}
	template <typename OtherKey>
	iterator find(const OtherKey &key) {
		return this->template findFirst<OtherKey>(key);
	}
	template <typename OtherKey>
	const_iterator find(const OtherKey &key) const {
		return this->template findFirst<OtherKey>(key);
	}

	Type &operator[](const Key &key) {
		if (this->empty() || this->compare()(key, this->front().first)) {
			this->impl().push_front({ key, Type() });
			return this->front().second;
		} else if (this->compare()(this->back().first, key)) {
			this->impl().push_back({ key, Type() });
			return this->back().second;
		}
		auto where = this->getLowerBound(key);
		if (this->compare()(key, where->first)) {
			return this->impl().insert(where, { key, Type() })->second;
		}
		return where->second;
	}

	std::optional<Type> take(const Key &key) {
		auto it = find(key);
		if (it == this->end()) {
			return std::nullopt;
		}
		auto result = std::move(it->second);
		this->erase(it);
		return result;
	}

private:
	void finalize() {
		this->impl().erase(
			std::unique(
				std::begin(this->impl()),
				std::end(this->impl()),
				[&](auto &&a, auto &&b) {
					return !this->compare()(a, b);
				}
			),
			std::end(this->impl()));
	}

};

} // namespace base

// Structured bindings support.
namespace std {

template <typename Key, typename Value>
class tuple_size<base::flat_multi_map_pair_type<Key, Value>>
: public integral_constant<size_t, 2> {
};

template <typename Key, typename Value>
class tuple_element<0, base::flat_multi_map_pair_type<Key, Value>> {
public:
	using type = const Key;
};

template <typename Key, typename Value>
class tuple_element<1, base::flat_multi_map_pair_type<Key, Value>> {
public:
	using type = Value;
};

} // namespace std

// Structured bindings support.
namespace base {
namespace details {

template <std::size_t N, typename Key, typename Value>
using flat_multi_map_pair_element = std::tuple_element_t<
	N,
	flat_multi_map_pair_type<Key, Value>>;

} // namespace details

template <std::size_t N, typename Key, typename Value>
auto get(base::flat_multi_map_pair_type<Key, Value> &value)
-> details::flat_multi_map_pair_element<N, Key, Value> & {
	if constexpr (N == 0) {
		return value.first;
	} else {
		return value.second;
	}
}

template <std::size_t N, typename Key, typename Value>
auto get(const base::flat_multi_map_pair_type<Key, Value> &value)
-> const details::flat_multi_map_pair_element<N, Key, Value> & {
	if constexpr (N == 0) {
		return value.first;
	} else {
		return value.second;
	}
}

} // namespace base

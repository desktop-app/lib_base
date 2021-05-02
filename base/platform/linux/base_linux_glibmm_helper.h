// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include <tuple>
#include <glibmm/variant.h>

namespace base {
namespace Platform {

template <typename T>
auto MakeGlibVariant(T &&data) {
	return Glib::Variant<T>::create(data);
}

template <typename T>
auto GlibVariantCast(const Glib::VariantBase &data) {
	return Glib::VariantBase::cast_dynamic<Glib::Variant<T>>(data).get();
}

} // namespace Platform
} // namespace base

#ifndef DESKTOP_APP_USE_PACKAGED
// taken from https://github.com/GNOME/glibmm/commit/6e205b721ebb26264bb00bdf294e04255de34f34
namespace Glib {

/** Specialization of Variant containing a tuple.
 * @newin{2,52}
 * @ingroup Variant
 */
template <class... Types>
class Variant<std::tuple<Types...>> : public VariantContainerBase
{
public:
	using CppContainerType = std::tuple<Types...>;

	/// Default constructor
	Variant<std::tuple<Types...>>()
	: VariantContainerBase()
	{}

	/** GVariant constructor.
	 * @param castitem The GVariant to wrap.
	 * @param take_a_reference Whether to take an extra reference of the GVariant
	 *        or not (not taking one could destroy the GVariant with the wrapper).
	 */
	explicit Variant<std::tuple<Types...>>(GVariant* castitem, bool take_a_reference = false)
	: VariantContainerBase(castitem, take_a_reference)
	{}

	/** Creates a new Variant containing a tuple.
	 * @param data The tuple to use for creation.
	 * @return The new Variant holding a tuple.
	 * @newin{2,52}
	 */
	static Variant<std::tuple<Types...>> create(const std::tuple<Types...>& data);

	/** Gets the VariantType.
	 * @return The VariantType.
	 * @newin{2,52}
	 */
	static const VariantType& variant_type() G_GNUC_CONST;

	/** Gets a specific element from the tuple.
	 * It is an error if @a index is greater than or equal to the number of
	 * elements in the tuple. See VariantContainerBase::get_n_children().
	 *
	 * @param index The index of the element.
	 * @return The tuple element at index @a index.
	 * @throw std::out_of_range
	 * @newin{2,52}
	 */
	template<class T>
	T get_child(gsize index) const;

	template<class T>
	Variant<T> get_child_variant(gsize index) const;

	/** Gets the tuple of the Variant.
	 * @return The tuple.
	 * @newin{2,52}
	 */
	std::tuple<Types...> get() const;

	/** Gets a VariantIter of the Variant.
	 * @return The VariantIter.
	 * @newin{2,52}
	 */
	VariantIter get_iter() const;
};

/*---------------------Variant<std::tuple<class... Types>> --------------------*/

// static
template <class... Types>
const VariantType& Variant<std::tuple<Types...>>::variant_type()
{
	std::vector<VariantType> types;
	auto expander = [&types](const VariantType &type) mutable -> int
	{
		types.push_back(type);
		return 0;
	};

	// expands the variadic template parameters
	using swallow = int[]; // ensures left to right order
	swallow{(expander(Variant<Types>::variant_type()))...};
	static auto type = VariantType::create_tuple(types);

	return type;
}

namespace detail
{
template <class Tuple, std::size_t... Is>
void expand_tuple(std::vector<VariantBase> &variants, const Tuple & t,
									std::index_sequence<Is...>)
{
	using swallow = int[]; // ensures left to right order
	auto expander = [&variants](const VariantBase &variant) -> int
	{
		variants.push_back(variant);
		return 0;
	};
	(void)swallow {(expander(Variant<typename std::tuple_element<Is, Tuple>::type>::create(std::get<Is>(t))))...};
}
} // namespace detail

template <class... Types>
Variant<std::tuple<Types...>>
Variant<std::tuple<Types...>>::create(const std::tuple<Types...>& data)
{
	// create a vector containing all tuple values as variants
	std::vector<Glib::VariantBase> variants;
	detail::expand_tuple(variants, data, std::index_sequence_for<Types...>{});

	using var_ptr = GVariant*;
	var_ptr var_array[sizeof... (Types)];

	for (std::vector<VariantBase>::size_type i = 0; i < variants.size(); i++)
		var_array[i] = const_cast<GVariant*>(variants[i].gobj());

	Variant<std::tuple<Types...>> result = Variant<std::tuple<Types...>>(
					g_variant_new_tuple(var_array, variants.size()));

	return result;
}

template <class... Types>
template <class T>
T Variant<std::tuple<Types...>>::get_child(gsize index) const
{
	Variant<T> entry;
	VariantContainerBase::get_child(entry, index);
	return entry.get();
}

template <class... Types>
template <class T>
Variant<T> Variant<std::tuple<Types...>>::get_child_variant(gsize index) const
{
	Variant<T> entry;
	VariantContainerBase::get_child(entry, index);
	return entry;
}

namespace detail
{
// swallows any argument
template <class T>
constexpr int any_arg(T&& arg)
{
	(void)arg;
	return 0;
}

template <class Tuple, std::size_t... Is>
void assign_tuple(std::vector<VariantBase> &variants, Tuple & t, std::index_sequence<Is...>)
{
	int i = 0;
	using swallow = int[]; // ensures left to right order
	(void)swallow {(any_arg(std::get<Is>(t) = VariantBase::cast_dynamic<Variant<typename std::tuple_element<Is, Tuple>::type > >(variants[i++]).get()))...};
}
} // namespace detail

template <class... Types>
std::tuple<Types...> Variant<std::tuple<Types...>>::get() const
{
	std::tuple<Types...> data;
	int i = 0;

	std::vector<VariantBase> variants;
	using swallow = int[]; // ensures left to right order
	auto expander = [&variants, &i](const VariantBase &variant) -> int
	{
		variants.push_back(variant);
		return i++;
	};
	swallow{(expander(get_child_variant<Types>(i)))...};
	detail::assign_tuple(variants, data, std::index_sequence_for<Types...>{});

	return data;
}

template< class... Types>
VariantIter Variant<std::tuple<Types...>>::get_iter() const
{
	// Get the variant type of the elements.
	VariantType element_variant_type = Variant<Types...>::variant_type();

	// Get the variant type of the array.
	VariantType array_variant_type = Variant< std::tuple<Types...> >::variant_type();

	// Get the GVariantIter.
	GVariantIter* g_iter = 0;
	g_variant_get(const_cast<GVariant*>(gobj()),
		reinterpret_cast<gchar*>(array_variant_type.gobj()), &g_iter);

	return VariantIter(g_iter);
}

} // namespace Glib
#endif // !DESKTOP_APP_USE_PACKAGED

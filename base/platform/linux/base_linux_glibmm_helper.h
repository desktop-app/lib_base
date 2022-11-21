// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "base/basic_types.h"

#include <glibmm/variant.h>

namespace Glib {

template <>
class Variant<int64> : public VariantBase
{
public:
	using CType = gint64;

	/// Default constructor.
	Variant() : VariantBase() {
	}

	/** GVariant constructor.
	* @param castitem The GVariant to wrap.
	* @param take_a_reference Whether to take an extra reference of the
	* GVariant or not (not taking one could destroy the GVariant with the
	* wrapper).
	*/
	explicit Variant(GVariant *castitem, bool take_a_reference = false)
	: VariantBase(castitem, take_a_reference) {
	}

	/** Gets the Glib::VariantType.
	* @return The Glib::VariantType.
	*/
	static const VariantType &variant_type() G_GNUC_CONST {
		static VariantType type(G_VARIANT_TYPE_INT64);
		return type;
	}

	/** Creates a new Glib::Variant<int64>.
	* @param data The value of the new Glib::Variant<int64>.
	* @return The new Glib::Variant<int64>.
	*/
	static Variant<int64> create(int64 data) {
		auto result = Variant<int64>(g_variant_new_int64(data));
		return result;
	}

	/** Gets the value of the Glib::Variant<int64>.
	* @return The int64 value of the Glib::Variant<int64>.
	*/
	int64 get() const {
		return g_variant_get_int64(gobject_);
	}
};

template <>
class Variant<uint64> : public VariantBase
{
public:
	using CType = guint64;

	/// Default constructor.
	Variant() : VariantBase() {
	}

	/** GVariant constructor.
	* @param castitem The GVariant to wrap.
	* @param take_a_reference Whether to take an extra reference of the
	* GVariant or not (not taking one could destroy the GVariant with the
	* wrapper).
	*/
	explicit Variant(GVariant *castitem, bool take_a_reference = false)
	: VariantBase(castitem, take_a_reference) {
	}

	/** Gets the Glib::VariantType.
	* @return The Glib::VariantType.
	*/
	static const VariantType &variant_type() G_GNUC_CONST {
		static VariantType type(G_VARIANT_TYPE_UINT64);
		return type;
	}

	/** Creates a new Glib::Variant<uint64>.
	* @param data The value of the new Glib::Variant<uint64>.
	* @return The new Glib::Variant<uint64>.
	*/
	static Variant<uint64> create(uint64 data) {
		auto result = Variant<uint64>(g_variant_new_uint64(data));
		return result;
	}

	/** Gets the value of the Glib::Variant<uint64>.
	* @return The uint64 value of the Glib::Variant<uint64>.
	*/
	uint64 get() const {
		return g_variant_get_uint64(gobject_);
	}
};

} // namespace Glib

namespace base {
namespace Platform {

template <typename T>
auto MakeGlibVariant(T &&data) {
	return Glib::Variant<std::decay_t<T>>::create(data);
}

template <typename T>
auto MakeGlibVariant(const T &data) {
	return Glib::Variant<T>::create(data);
}

template <typename T>
auto GlibVariantCast(const Glib::VariantBase &data) {
	return Glib::VariantBase::cast_dynamic<Glib::Variant<T>>(data).get();
}

} // namespace Platform
} // namespace base

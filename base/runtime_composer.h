// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

template <typename Base>
class RuntimeComposer;

class RuntimeComposerBase;
typedef void(*RuntimeComponentConstruct)(void *location, RuntimeComposerBase *composer);
typedef void(*RuntimeComponentDestruct)(void *location);
typedef void(*RuntimeComponentMove)(void *location, void *waslocation);

struct RuntimeComponentWrapStruct {
	// Don't init any fields, because it is only created in
	// global scope, so it will be filled by zeros from the start.
	RuntimeComponentWrapStruct() = default;
	RuntimeComponentWrapStruct(std::size_t size, std::size_t align, RuntimeComponentConstruct construct, RuntimeComponentDestruct destruct, RuntimeComponentMove move)
		: Size(size)
		, Align(align)
		, Construct(construct)
		, Destruct(destruct)
		, Move(move) {
	}
	std::size_t Size;
	std::size_t Align;
	RuntimeComponentConstruct Construct;
	RuntimeComponentDestruct Destruct;
	RuntimeComponentMove Move;
};

template <int Value, int Denominator>
struct CeilDivideMinimumOne {
	static constexpr int Result = ((Value / Denominator) + ((!Value || (Value % Denominator)) ? 1 : 0));
};

// Components are registered in per-Base tables, so each
// RuntimeComposer<Base> hierarchy has its own limit of 64 components.
template <typename Base>
RuntimeComponentWrapStruct RuntimeComponentWraps[64];

template <typename Base>
QAtomicInt RuntimeComponentIndexLast;

template <typename Type, typename Base>
struct RuntimeComponent {
	using RuntimeComponentBase = Base;

	RuntimeComponent() {
		// While there is no std::aligned_alloc().
		static_assert(alignof(Type) <= alignof(std::max_align_t), "Components should align to std::max_align_t!");
	}
	RuntimeComponent(const RuntimeComponent &other) = delete;
	RuntimeComponent &operator=(const RuntimeComponent &other) = delete;
	RuntimeComponent(RuntimeComponent &&other) = delete;
	RuntimeComponent &operator=(RuntimeComponent &&other) = default;

	static int Index() {
		static QAtomicInt MyIndex(0);
		if (auto index = MyIndex.loadAcquire()) {
			return index - 1;
		}
		while (true) {
			auto last = RuntimeComponentIndexLast<Base>.loadAcquire();
			if (RuntimeComponentIndexLast<Base>.testAndSetOrdered(last, last + 1)) {
				Assert(last < 64);
				if (MyIndex.testAndSetOrdered(0, last + 1)) {
					RuntimeComponentWraps<Base>[last] = RuntimeComponentWrapStruct(
						sizeof(Type),
						alignof(Type),
						Type::RuntimeComponentConstruct,
						Type::RuntimeComponentDestruct,
						Type::RuntimeComponentMove);
				}
				break;
			}
		}
		return MyIndex.loadAcquire() - 1;
	}
	static uint64 Bit() {
		return (1ULL << Index());
	}

protected:
	static void RuntimeComponentConstruct(void *location, RuntimeComposerBase *composer) {
		new (location) Type();
	}
	static void RuntimeComponentDestruct(void *location) {
		((Type*)location)->~Type();
	}
	static void RuntimeComponentMove(void *location, void *waslocation) {
		*(Type*)location = std::move(*(Type*)waslocation);
	}

};

class RuntimeComposerMetadata {
public:
	RuntimeComposerMetadata(
		uint64 mask,
		const RuntimeComponentWrapStruct *wraps)
	: wraps(wraps)
	, _mask(mask) {
		for (int i = 0; i != 64; ++i) {
			auto componentBit = (1ULL << i);
			if (_mask & componentBit) {
				auto componentSize = wraps[i].Size;
				if (componentSize) {
					auto componentAlign = wraps[i].Align;
					if (auto badAlign = (size % componentAlign)) {
						size += (componentAlign - badAlign);
					}
					offsets[i] = size;
					size += componentSize;
					accumulate_max(align, componentAlign);
				}
			} else if (_mask < componentBit) {
				last = i;
				break;
			}
		}
	}

	// The per-Base components registry this metadata was built from.
	const RuntimeComponentWrapStruct *wraps = nullptr;

	// Meta pointer in the start.
	std::size_t size = sizeof(const RuntimeComposerMetadata*);
	std::size_t align = alignof(const RuntimeComposerMetadata*);
	std::size_t offsets[64] = { 0 };
	int last = 64;

	bool equals(uint64 mask) const {
		return _mask == mask;
	}
	uint64 maskadd(uint64 mask) const {
		return _mask | mask;
	}
	uint64 maskremove(uint64 mask) const {
		return _mask & (~mask);
	}

private:
	uint64 _mask;

};

const RuntimeComposerMetadata *GetRuntimeComposerMetadata(
	uint64 mask,
	const RuntimeComponentWrapStruct *wraps);

class RuntimeComposerBase {
public:
	explicit RuntimeComposerBase(const RuntimeComposerMetadata *meta = nullptr)
	: _data(zerodata()) {
		if (meta) {
			auto data = operator new(meta->size);
			Assert(data != nullptr);

			_data = data;
			_meta() = meta;
			for (int i = 0; i < meta->last; ++i) {
				auto offset = meta->offsets[i];
				if (offset >= sizeof(_meta())) {
					try {
						auto constructAt = _dataptrunsafe(offset);
						auto space = meta->wraps[i].Size;
						auto alignedAt = constructAt;
						std::align(meta->wraps[i].Align, space, alignedAt, space);
						Assert(alignedAt == constructAt);
						meta->wraps[i].Construct(constructAt, this);
					} catch (...) {
						while (i > 0) {
							offset = meta->offsets[--i];
							if (offset >= sizeof(_meta())) {
								meta->wraps[i].Destruct(_dataptrunsafe(offset));
							}
						}
						throw;
					}
				}
			}
		}
	}
	RuntimeComposerBase(const RuntimeComposerBase &other) = delete;
	RuntimeComposerBase &operator=(const RuntimeComposerBase &other) = delete;
	~RuntimeComposerBase() {
		if (_data != zerodata()) {
			auto meta = _meta();
			for (int i = 0; i < meta->last; ++i) {
				auto offset = meta->offsets[i];
				if (offset >= sizeof(_meta())) {
					meta->wraps[i].Destruct(_dataptrunsafe(offset));
				}
			}
			operator delete(_data);
		}
	}

protected:
	bool UpdateComponents(const RuntimeComposerMetadata *meta, uint64 mask) {
		if (_meta()->equals(mask)) {
			return false;
		}
		RuntimeComposerBase result(meta);
		result.swap(*this);
		if (_data != zerodata() && result._data != zerodata()) {
			const auto nowmeta = _meta();
			const auto wasmeta = result._meta();
			for (auto i = 0; i != nowmeta->last; ++i) {
				const auto offset = nowmeta->offsets[i];
				const auto wasoffset = wasmeta->offsets[i];
				if (offset >= sizeof(_meta())
					&& wasoffset >= sizeof(_meta())) {
					nowmeta->wraps[i].Move(
						_dataptrunsafe(offset),
						result._dataptrunsafe(wasoffset));
				}
			}
		}
		return true;
	}

private:
	template <typename Base>
	friend class RuntimeComposer;

	static const RuntimeComposerMetadata *ZeroRuntimeComposerMetadata;
	static void *zerodata() {
		return &ZeroRuntimeComposerMetadata;
	}

	void *_dataptrunsafe(int skip) const {
		return (char*)_data + skip;
	}
	void *_dataptr(int skip) const {
		return (skip >= sizeof(_meta())) ? _dataptrunsafe(skip) : nullptr;
	}
	const RuntimeComposerMetadata *&_meta() const {
		return *static_cast<const RuntimeComposerMetadata**>(_data);
	}
	void *_data = nullptr;

	void swap(RuntimeComposerBase &other) {
		std::swap(_data, other._data);
	}

};

template <typename Base>
class RuntimeComposer : public RuntimeComposerBase {
public:
	RuntimeComposer(uint64 mask = 0)
	: RuntimeComposerBase(MetadataFor(mask)) {
	}

	template <
		typename Type,
		typename = std::enable_if_t<std::is_same_v<
			typename Type::RuntimeComponentBase,
			Base>>>
	bool Has() const {
		return (_meta()->offsets[Type::Index()] >= sizeof(_meta()));
	}

	template <
		typename Type,
		typename = std::enable_if_t<std::is_same_v<
			typename Type::RuntimeComponentBase,
			Base>>>
	Type *Get() {
		return static_cast<Type*>(_dataptr(_meta()->offsets[Type::Index()]));
	}
	template <
		typename Type,
		typename = std::enable_if_t<std::is_same_v<
			typename Type::RuntimeComponentBase,
			Base>>>
	const Type *Get() const {
		return static_cast<const Type*>(_dataptr(_meta()->offsets[Type::Index()]));
	}

protected:
	bool UpdateComponents(uint64 mask = 0) {
		if (_meta()->equals(mask)) {
			return false;
		}
		return RuntimeComposerBase::UpdateComponents(MetadataFor(mask), mask);
	}
	bool AddComponents(uint64 mask = 0) {
		return UpdateComponents(_meta()->maskadd(mask));
	}
	bool RemoveComponents(uint64 mask = 0) {
		return UpdateComponents(_meta()->maskremove(mask));
	}

private:
	static const RuntimeComposerMetadata *MetadataFor(uint64 mask) {
		return mask
			? GetRuntimeComposerMetadata(mask, RuntimeComponentWraps<Base>)
			: nullptr;
	}

};

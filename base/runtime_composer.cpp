// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/runtime_composer.h"

struct RuntimeComposerMetadatasMap {
	std::map<
		std::pair<uint64, const RuntimeComponentWrapStruct*>,
		std::unique_ptr<RuntimeComposerMetadata>> data;
	QMutex mutex;
};

const RuntimeComposerMetadata *GetRuntimeComposerMetadata(
		uint64 mask,
		const RuntimeComponentWrapStruct *wraps) {
	static RuntimeComposerMetadatasMap RuntimeComposerMetadatas;

	QMutexLocker lock(&RuntimeComposerMetadatas.mutex);
	const auto key = std::make_pair(mask, wraps);
	auto i = RuntimeComposerMetadatas.data.find(key);
	if (i == end(RuntimeComposerMetadatas.data)) {
		i = RuntimeComposerMetadatas.data.emplace(
			key,
			std::make_unique<RuntimeComposerMetadata>(mask, wraps)).first;
	}
	return i->second.get();
}

const RuntimeComposerMetadata *RuntimeComposerBase::ZeroRuntimeComposerMetadata = GetRuntimeComposerMetadata(0, nullptr);

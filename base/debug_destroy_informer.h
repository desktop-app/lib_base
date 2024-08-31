// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//

#ifdef _DEBUG

#pragma once

#include <iostream>

template <typename T>
class PrintDead : public T {
public:
	template <typename... Args>
	PrintDead(Args&&... args) : T(std::forward<Args>(args)...) {}

	~PrintDead() {
		std::cout
			<< "\033[32m" // Green.
			<< "Instance of class "
			<< typeid(T).name()
			<< " is dead!"
			<< "\033[0m"
			<< std::endl;
	}
};

// #include "base/debug_destroy_informer.h"
// Usage: std::make_shared<PrintDead<QSvgRenderer>>(...);
// Usage: PrintDead<QSvgRenderer>(...);

#endif // _DEBUG

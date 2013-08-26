#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <sstream>
#include <memory>
#include "token.h"

template<class T>
struct str_to_num {
	T operator()(const std::string& str) {
		T num;
		std::stringstream ss(str);
		ss >> num;
		return num;
	}
};

//template<class T, class... Args>
//std::unique_ptr<T> make_unique(Args&&... args);

std::nullptr_t error(const std::string& str);
bool is_binop(int c);
bool is_binop(const std::string& s);
bool is_cmp(int c);
bool is_toplvl(int t);

#endif

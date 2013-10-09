#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <sstream>
#include <memory>
#include <vector>
#include <set>
#include <queue>

class ast;
class ast_func;

template<class T>
struct str_to_num {
	T operator()(const std::string& str) {
		T num;
		std::stringstream ss(str);
		ss >> num;
		return num;
	}
};

bool is_binop(int c);
bool is_binop(const std::string& s);
bool is_cmp(int c);
void print_node(const std::shared_ptr<ast>& node);
//void print_nodes(std::queue<std::shared_ptr<ast>>& nodes);
int get_num_args(const std::shared_ptr<ast_func>& f);

template<class T>
bool is_unique_vector(const std::vector<T>& v)
{
    std::set<T> s(v.begin(), v.end());
    return v.size() == s.size();
}

#endif

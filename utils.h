#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <sstream>
#include <memory>
#include <vector>
#include <set>
#include <queue>
#include <functional>
#include "ast_detail.h"

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
//void print_node(const std::shared_ptr<ast>& node);
//void print_nodes(std::queue<std::shared_ptr<ast>>& nodes);
//int get_num_args(const std::shared_ptr<ast_func>& f);

template<class T>
bool is_unique_vector(const std::vector<T>& v)
{
    std::set<T> s(v.begin(), v.end());
    return v.size() == s.size();
}

namespace sexp
{
    enum class form_type {FUNCTION, INVALID};
    enum class func_err {INVALID_CAR, INVALID_CDR, INVALID_PROTO, INVALID_BODY, INVALID_FIRST_STR, VALID};

    void map_effect(std::function<void(const shared_ast&)> f, shared_ast list);
    std::size_t get_length(const shared_ast& a, std::size_t accum = 1);

    // ensure a is func
    shared_ast get_body(const ast& a);
    shared_ast get_proto(const ast& a);
    std::string get_func_name(const ast& a);

    std::string get_first_str(const ast& a);
    bool check_first_str(const ast& a, const std::string& s);

    bool is_function(const ast_cons& a);
    func_err validate_func(const ast_cons& a);
}

#endif

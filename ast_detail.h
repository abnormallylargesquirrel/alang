#ifndef AST_DETAIL_H
#define AST_DETAIL_H

#include <cstdint>

namespace tags
{
static const std::size_t tstr = 0;
static const std::size_t tint = 1;
static const std::size_t tfloat = 2;
static const std::size_t tcons = 3;
static const std::size_t tsym = 4;
}

class ast;
class ast_str;
class ast_int;
class ast_float;
class ast_cons;
class ast_sym;

typedef std::shared_ptr<ast> shared_ast;
typedef std::shared_ptr<ast_str> shared_str;
typedef std::shared_ptr<ast_cons> shared_cons;
typedef std::shared_ptr<ast_int> shared_int;
typedef std::shared_ptr<ast_float> shared_float;
typedef std::shared_ptr<ast_sym> shared_sym;

#endif

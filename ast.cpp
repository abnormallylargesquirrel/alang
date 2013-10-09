#include "ast.h"
#include "hm_inference.h"

type ast::infer_type(inferencer& inf)
{
    return inf(*this);
}
type expr_int::infer_type(inferencer& inf)
{
    return inf(*this);
}
type expr_float::infer_type(inferencer& inf)
{
    return inf(*this);
}
type expr_sym::infer_type(inferencer& inf)
{
    return inf(*this);
}
type expr_apply::infer_type(inferencer& inf)
{
    return inf(*this);
}
type expr_if::infer_type(inferencer& inf)
{
    return inf(*this);
}
type expr_binop::infer_type(inferencer& inf)
{
    return inf(*this);
}
type ast_func::infer_type(inferencer& inf)
{
    return inf(*this);
}

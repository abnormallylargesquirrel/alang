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
/*type binop_add::infer_type(inferencer& inf)
{
    return inf(*this);
}
type binop_sub::infer_type(inferencer& inf)
{
    return inf(*this);
}
type binop_mul::infer_type(inferencer& inf)
{
    return inf(*this);
}
type binop_div::infer_type(inferencer& inf)
{
    return inf(*this);
}
type binop_lt::infer_type(inferencer& inf)
{
    return inf(*this);
}
type binop_gt::infer_type(inferencer& inf)
{
    return inf(*this);
}
type binop_lte::infer_type(inferencer& inf)
{
    return inf(*this);
}
type binop_gte::infer_type(inferencer& inf)
{
    return inf(*this);
}
type binop_eq::infer_type(inferencer& inf)
{
    return inf(*this);
}*/
type ast_func::infer_type(inferencer& inf)
{
    return inf(*this);
}

#ifndef EVAL_T_H
#define EVAL_T_H

//when child nodes have multiple types, node type will be greatest val (ev_float + ev_int64 -> ev_int64)
enum class eval_t {ev_invalid, ev_void, ev_float, ev_int64};

static const char *eval_strings[] = {"ev_invalid", "ev_void", "ev_float", "ev_int64"};

#endif

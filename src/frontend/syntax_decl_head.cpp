#include <frontend/syntax_decl_head.hpp>

syntax_decl_head::syntax_decl_head(basic_type b_type, bool is_const)
    : is_const(is_const),
      type(b_type)
{
}
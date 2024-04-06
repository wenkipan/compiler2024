#include <frontend/syntax_decl.hpp>

syntax_decl::syntax_decl(char *name)
    : name(new char[strlen(name) + 1]),
      p_array(NULL),
      p_init(NULL)
{
    strcpy(this->name, name);
}
// maybe memory leak
//??? problem
p_syntax_decl syntax_decl::syntax_decl_arr(p_ast_exp p_exp)
{
    size_t size = 0;
    if (p_exp)
    { // TODO
        p_exp->ast_exp_ptr_to_val_check_basic();
        assert(p_exp->p_type->basic == type_i32);
        assert(p_exp->i32const > 0);
        size = p_exp->i32const;
        delete (p_exp);
    }
    p_syntax_type_array p_arrary = new syntax_type_array(size);
    this->p_array = this->p_array->syntax_type_add_array(p_arrary);

    return this;
}
p_syntax_decl syntax_decl::syntax_decl_init(p_syntax_init p_init)
{
    this->p_init = p_init;
    return this;
}
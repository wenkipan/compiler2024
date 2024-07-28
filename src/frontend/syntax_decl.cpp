#include "../../include/ast/ast.hpp"
#include "../../include/frontend/syntax_decl.hpp"

syntax_decl::syntax_decl(char *name)
    : name(new char[strlen(name) + 1]),
      p_array(NULL),
      p_init(NULL)
{
    strcpy(this->name, name);
}
syntax_decl::~syntax_decl()
{
    delete[] name;
}
// maybe memory leak
//??? problem
p_syntax_decl syntax_decl::syntax_decl_arr(p_ast_exp p_exp)
{

    size_t size = 0;
    if (p_exp)
    { // TODO
        p_ast_exp _exp = p_exp->ast_exp_ptr_to_val_check_basic();
        assert(_exp);
        assert(_exp->p_type->basic == type_i32);
        assert(_exp->i32const > 0);
        size = _exp->i32const;
        ast_exp_drop(_exp);
    }
    // printf("syntax_decl_arr::%lu\n", size);
    p_syntax_type_array p_arrary = new syntax_type_array(size);
    this->p_array = this->p_array->syntax_type_add_array(p_arrary);

    return this;
}
p_syntax_decl syntax_decl::syntax_decl_init(p_syntax_init p_init)
{
    this->p_init = p_init;
    // printf("%s", this->name, this->p_init->);
    //  printf("syntax_decl_init%u\n", this->);
    return this;
}
#pragma once
#include "symbol/func.hpp"
#include <ast/ast.hpp>
#include <ir/ir.hpp>
class ast2ir
{
    Module *module;

public:
    ast2ir();
    Module *get_module() { return module; }
    void ast2ir_add_function(p_symbol_func p_func, p_ast_block p_h_block);
    void function_add_params(p_symbol_func p_func);
};
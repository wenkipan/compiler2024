#pragma once

#include <ir.h>
class ir_param
{
    bool is_in_mem;
    union
    {
        p_symbol_var p_vmem;
        p_ir_operand p_param;
    };
    list_head node;
};
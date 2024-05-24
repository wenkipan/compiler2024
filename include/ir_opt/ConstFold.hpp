#pragma once

#include "ir/Instrution.hpp"
#include <ir/ir.hpp>

Value *const_fold(Instrution *instr);
Value *const_fold_binary(Binary *instr);
Value *const_fold_Cmp(Cmp *instr);
Value *const_fold_unary(Unary *instr);
#pragma once

#include "../../include/lir/lir.hpp"

#define imm_8_max 255
#define imm_12_max 4095
#define imm_16_max 65535

bool is_legal_rotate_imme(int a);

bool is_legal_ldr_str_imme(int a);

static inline bool is_r_reg(int no) { return 0 <= no && no <= 15; }

static inline bool is_s_reg(int no) { return no > 15 && no <= 15 + 32; }
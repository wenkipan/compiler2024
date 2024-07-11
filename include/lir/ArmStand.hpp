#pragma once

#include <lir/immeFloatToLoad.hpp>
#include <lir/immeIntTomove.hpp>
#include <lir/LargeToGlobal.hpp>
#include <lir/GEPToALU.hpp>
#include <lir/modTosubmul.hpp>

#define imm_8_max 255
#define imm_12_max 4095
#define imm_16_max 65535

bool is_legal_rotate_imme(int a);
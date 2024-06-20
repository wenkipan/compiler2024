#pragma once

#include <lir/immeFloatToLoad.hpp>
#include <lir/immeIntTomove.hpp>
#include <lir/LargeToGlobal.hpp>
#include <lir/GEPToALU.hpp>

bool is_legal_rotate_imme(int a);
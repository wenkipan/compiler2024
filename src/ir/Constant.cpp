#include <ir/Constant.hpp>

Constant::Constant(TypeEnum type)
    : User(type)
{
}
Constant::Constant(Type *ty)
    : User(ty, true)
{
}

ConstantI32::ConstantI32(p_symbol_init p_init)
    : Constant(TypeEnum::Array)
{
    for (int i = 0; i < p_init->size; ++i)
        i32.emplace_back(p_init->memory[i].i);
}

ConstantI32::ConstantI32(I32CONST_t _I32)
    : Constant(TypeEnum::I32), i32(1, _I32)
{
}
ConstantI32::ConstantI32(Type *type, const std::vector<int> &vi)
    : Constant(type), i32(vi)
{
}
ConstantI32::ConstantI32(ConstantI32 *ci32)
    : Constant(TypeEnum::I32), i32(ci32->get_i32())
{
}
int ConstantI32::get_32_at(int pos)
{
    assert(pos < i32.size() && pos >= 0);
    return i32[pos];
}

ConstantF32::ConstantF32(p_symbol_init p_init)
    : Constant(TypeEnum::Array)
{
    for (int i = 0; i < p_init->size; ++i)
        f32.emplace_back(p_init->memory[i].f);
}

ConstantF32::ConstantF32(F32CONST_t _F32)
    : Constant(TypeEnum::F32), f32(1, _F32)
{
}
ConstantF32::ConstantF32(ConstantF32 *cf32)
    : Constant(TypeEnum::F32), f32(cf32->get_f32())
{
}
ConstantF32::ConstantF32(Type *type, const std::vector<float> &vi)
    : Constant(type), f32(vi)
{
}
float ConstantF32::get_32_at(int pos)
{
    assert(pos < f32.size() && pos >= 0);
    return f32[pos];
}

ConstantStr::ConstantStr(const char *_str)
    : Constant(TypeEnum::Str), str(_str, strlen(_str))
{
}

void ConstantI32::print_ID()
{
    if (i32.size() == 1)
        printf("%d", i32[0]);
    else
        printf("%%%d", this->get_ID());
}

static void _rec_printI32(int _pos, ArrayType *_array, int &_nw, std::vector<int> &_i32)
{
    // printf("pos = %d, nw = %d\n", _pos, _nw);
    if (_pos == _array->get_dims()->size() - 1)
    {
        _array->print(_pos);
        putchar(' ');
        int n = (*_array->get_dims())[_pos] - 1;
        putchar('[');
        for (int i = 0; i < n; ++i)
            printf("i32 %d, ", _i32[_nw++]);
        printf("i32 %d]", _i32[_nw++]);
        return;
    }
    _array->print(_pos);
    putchar(' ');
    putchar('[');
    int n = (*_array->get_dims())[_pos] - 1;
    for (int i = 0; i < n; ++i)
    {
        _rec_printI32(_pos + 1, _array, _nw, _i32);
        printf(", ");
    }
    _rec_printI32(_pos + 1, _array, _nw, _i32);
    putchar(']');
}

void ConstantI32::llvm_print(Type *_type)
{
    assert(_type->get_type() == TypeEnum::Array && _type->get_basic_type() == TypeEnum::I32);
    ArrayType *_array = static_cast<ArrayType *>(_type);
    std::vector<int> *_dims = _array->get_dims();
    int n = _dims->size() - 1;
    int nw = 0;
    assert(i32.size() == _array->get_size());
    _rec_printI32(0, _array, nw, i32);
    if (!n)
        printf(", align 4\n");
    else
        printf(", align 16\n");
}

void ConstantF32::print_ID()
{
    if (f32.size() == 1)
    {
        double nw = f32[0];
        unsigned long long *ptr = (unsigned long long *)&(nw);
        printf("0x%llX", *ptr);
    }
    else
        printf("%%%d", this->get_ID());
}

static void _rec_printF32(int _pos, ArrayType *_array, int &_nw, std::vector<float> &_f32)
{
    if (_pos == _array->get_dims()->size() - 1)
    {
        _array->print(_pos);
        putchar(' ');
        int n = (*_array->get_dims())[_pos] - 1;
        putchar('[');
        double _num;
        for (int i = 0; i < n; ++i)
        {
            _num = _f32[_nw++];
            unsigned long long *ptr = (unsigned long long *)&(_num);
            printf("float 0x%llX, ", *ptr);
        }
        _num = _f32[_nw++];
        unsigned long long *ptr = (unsigned long long *)&(_num);
        printf("float 0x%llX]", *ptr);
        return;
    }
    _array->print(_pos);
    putchar(' ');
    putchar('[');
    int n = (*_array->get_dims())[_pos] - 1;
    for (int i = 0; i < n; ++n)
    {
        _rec_printF32(_pos + 1, _array, _nw, _f32);
        printf(", ");
    }
    _rec_printF32(_pos + 1, _array, _nw, _f32);
    putchar(']');
}

void ConstantF32::llvm_print(Type *_type)
{
    assert(_type->get_type() == TypeEnum::Array && _type->get_basic_type() == TypeEnum::F32);
    ArrayType *_array = static_cast<ArrayType *>(_type);
    std::vector<int> *_dims = _array->get_dims();
    int n = _dims->size() - 1;
    int nw = 0;
    _rec_printF32(0, _array, nw, f32);
    if (!n)
        printf(", align 4\n");
    else
        printf(", align 16\n");
}

// drop

Constant::~Constant()
{
}

ConstantI32::~ConstantI32()
{
}

ConstantF32::~ConstantF32()
{
}

ConstantStr::~ConstantStr()
{
}

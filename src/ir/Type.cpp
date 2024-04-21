#include <ir/Type.hpp>

Type::Type(TypeEnum _type)
    : type(_type)
{
}

Type::Type(Type *p_type)
    : type(p_type->type)
{
}

Type::Type(basic_type mtype)
{
    switch (mtype)
    {
    case type_void:
        type = TypeEnum::Void;
        break;
    case type_f32:
        type = TypeEnum::F32;
        break;
    case type_i32:
        type = TypeEnum::I32;
        break;
    default:
        assert(0);
        break;
    }
}

TypeEnum Type::get_type()
{
    return type;
}

void Type::reset(TypeEnum _type)
{
    type = _type;
}

Ptr::Ptr(TypeEnum _type)
    : Type(TypeEnum::Ptr)
{
    b_type = new Type(_type);
}

Ptr::Ptr(ArrayType *_type)
    : Type(TypeEnum::Ptr)
{
    b_type = new ArrayType(_type);
}

Ptr::Ptr(p_symbol_type p_var)
    : Type(TypeEnum::Ptr)
{
    if (list_head_alone(&p_var->array))
        b_type = new Type(p_var->basic);
    else
        b_type = new ArrayType(p_var);
}

ArrayType::ArrayType(ArrayType *p_array)
    : Type(TypeEnum::Array),
      dims(new std::vector<int>)
{
    b_type = p_array->get_basictype();
    std::vector<int> *f_dims = p_array->get_dims();
    int n = f_dims->size();
    size = p_array->get_size() / ((*f_dims)[0]);
    for (int i = 1; i < n; ++i)
        dims->emplace_back((*f_dims)[i]);
}

ArrayType::ArrayType(p_symbol_type p_type)
    : Type(TypeEnum::Array),
      dims(new std::vector<int>)
{
    basic_type mtype = p_type->basic;
    switch (mtype)
    {
    case type_void:
        assert(0);
        break;
    case type_f32:
        b_type = TypeEnum::F32;
        break;
    case type_i32:
        b_type = TypeEnum::I32;
        break;
    default:
        assert(0);
        break;
    }

    size = 1;
    p_list_head p_node;
    list_for_each(p_node, &p_type->array)
    {
        p_symbol_type_array p_arry = list_entry(p_node, symbol_type_array, node);
        dims->emplace_back(p_arry->size);
        size *= p_arry->size;
    }
}

// print

void Type::print()
{
    switch (type)
    {
    case TypeEnum::Void:
        printf("void");
        break;
    case TypeEnum::F32:
        printf("f32");
        break;
    case TypeEnum::I32:
        printf("i32");
        break;
    case TypeEnum::Str:
        printf("str");
        break;
    default:
        assert(0);
        break;
    }
}

void Ptr::print()
{
    b_type->print();
    putchar('*');
}

void ArrayType::print()
{
    putchar('[');
    int n = dims->size() - 1;
    for (int i = 0; i < n; ++i)
        printf("%d x [", (*dims)[i]);
    printf("%d x ", (*dims)[n]);
    if (b_type == TypeEnum::I32)
        printf("i32");
    else
        printf("f32");
    for (int i = 0; i <= n; ++i)
        putchar(']');
}

// drop

Type::~Type()
{
}

ArrayType::~ArrayType()
{
    delete dims;
}

Ptr::~Ptr()
{
    delete b_type;
}
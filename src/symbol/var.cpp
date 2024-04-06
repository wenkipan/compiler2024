#include <symbol/var.hpp>

symbol_init::symbol_init(size_t size, basic_type basic)
    : basic(basic),
      size(size),
      memory(new symbol_init_val[size])
{
    memset(this->memory, 0, sizeof(*this->memory) * size);
}
symbol_init::~symbol_init()
{
    delete[] this->memory;
}
p_symbol_init symbol_init::symbol_init_add(size_t offset, symbol_init_val val)
{
    assert(offset < this->size);
    this->memory[offset] = val;
    return this;
}

symbol_var::symbol_var(const char *name, p_symbol_type p_type, bool is_const, bool is_global, p_symbol_init p_data)
    : p_type(p_type),
      name(new char[strlen(name) + 1]),
      id(0),
      p_init(p_data),
      is_global(is_global),
      is_const(is_const),
      node(list_head_init(&this->node))
{
    strcpy(this->name, name);
}
symbol_var::symbol_var(p_symbol_type p_type)
    : p_type(p_type),
      name(NULL),
      id(0),
      p_init(NULL),
      is_global(false),
      is_const(false),
      node(list_head_init(&this->node))
{
}

symbol_var::~symbol_var()
{
    list_del(&this->node);
    delete (this->p_init);
    delete (this->p_type);
    delete[] this->name;
}

void symbol_var::symbol_name_print()
{
    this->p_type->symbol_type_print();

    if (this->is_global)
    {
        printf(" @");
    }
    else
    {
        printf(" $");
    }
    printf("%ld", this->id);
    if (this->name)
    {
        printf("_%s", this->name);
    }
}

void symbol_var::symbol_init_print()
{
    this->p_type->symbol_type_print();
    printf(" ");
    this->symbol_name_print();
    if (!this->p_init)
    {
        printf("\n");
        return;
    }

    printf(" = {");
    for (size_t i = 0; i < this->p_init->size; ++i)
    {
        if (i > 0)
            printf(", ");
        if (this->p_init->basic == type_i32)
            printf("%d", this->p_init->memory[i].i);
        else
            printf("%f", this->p_init->memory[i].f);
    }
    printf("}\n");
}
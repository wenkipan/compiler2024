#include <frontend/symbol_table.hpp>

static inline size_t symbol_str_tag(const char *name)
{
    size_t hash = 0;
    const char *ptr = name;
    while (*ptr)
    {
        hash = hash * hash_P + *(ptr++);
    }
    return hash;
}

static inline hlist_hash init_hash()
{
    // maybe problem
    hlist_hash hash = new hlist_head[hash_MOD];
    for (size_t i = 0; i < hash_MOD; ++i)
        hlist_head_init(hash + i);
    return hash;
}

symbol_table::symbol_table()
    : hash(init_hash()),
      string_hash(init_hash()),
      level(0),
      p_top_table(NULL)
{
}
symbol_table::~symbol_table()
{
    assert(this->p_top_table == NULL);
    assert(this->level == 0);
    for (size_t i = 0; i < hash_MOD; ++i)
    {
        assert(hlist_head_empty(this->hash + i));
    }
    delete[] this->hash;
    delete[] this->string_hash;
}
void symbol_table::symbol_table_zone_pop()
{
    p_symbol_zone del_table = this->p_top_table;
    this->p_top_table = del_table->p_prev;
    --this->level;

    p_symbol_item p_item = del_table->p_item;
    while (p_item)
    {
        p_item->p_name->p_item = p_item->p_prev;
        if (!p_item->p_name->p_item)
        {
            // maybe problem cloud be ~symbol_name
            hlist_node_del(&p_item->p_name->node);
            delete[] (p_item->p_name->name);
            delete (p_item->p_name);
        }

        p_symbol_item del_item = p_item;
        p_item = p_item->p_next;
        delete (del_item);
    }

    delete (del_table);
}
void symbol_table::symbol_table_zone_push()
{
    p_symbol_zone pst = new symbol_zone();
    *pst = (symbol_zone){
        .p_item = NULL,
        .p_prev = this->p_top_table,
    };
    this->p_top_table = pst;
    ++this->level;
}
static inline p_symbol_name symbol_add_name(p_hlist_head p_head, size_t hash_tag, const char *name)
{
    p_symbol_name p_name = new symbol_name();
    *p_name = (symbol_name){
        .name = new char[strlen(name) + 1],
        .p_item = NULL,
        .hash_tag = hash_tag,
        .node = hlist_init_node,
    };
    strcpy(p_name->name, name);
    hlist_node_add(p_head, &p_name->node);
    return p_name;
}
static inline p_symbol_name symbol_find_name(p_hlist_head p_head, const char *name)
{
    p_hlist_node p_node;
    hlist_for_each(p_node, p_head)
    {
        p_symbol_name p_name = hlist_entry(p_node, symbol_name, node);
        if (!strcmp(p_name->name, name))
            return p_name;
    }
    return NULL;
}
static inline p_symbol_name symbol_table_get_name(p_symbol_table p_table, const char *name)
{
    size_t hash_tag = symbol_str_tag(name);
    p_hlist_head p_head = p_table->hash + (hash_tag % hash_MOD);

    p_symbol_name p_name = symbol_find_name(p_head, name);
    if (!p_name)
    {
        p_name = symbol_add_name(p_head, hash_tag, name);
    }
    else
    {
        assert(p_name->p_item->level < p_table->level);
    }

    return p_name;
}
p_symbol_item symbol_table::symbol_table_var_add(p_symbol_var p_var)
{
    assert(this->level > 0);

    p_symbol_name p_name = symbol_table_get_name(this, p_var->name);

    p_symbol_item p_item = new symbol_item();
    *p_item = (symbol_item){
        .p_name = p_name,
        .p_prev = p_name->p_item,
        .level = this->level,
        .p_next = this->p_top_table->p_item,
        .is_func = false,
        .p_var = p_var,
    };
    this->p_top_table->p_item = p_item;
    p_name->p_item = p_item;

    return p_item;
}
p_symbol_item symbol_table::symbol_table_func_add(p_symbol_func p_func)
{
    assert(this->level > 0);

    p_symbol_name p_name = symbol_table_get_name(this, p_func->name);

    p_symbol_item p_item = new symbol_item();
    *p_item = (symbol_item){
        .p_name = p_name,
        .p_prev = p_name->p_item,
        .level = this->level,
        .p_next = this->p_top_table->p_item,
        .is_func = true,
        .p_func = p_func,
    };
    this->p_top_table->p_item = p_item;
    p_name->p_item = p_item;

    return p_item;
}
p_symbol_var symbol_table::symbol_table_var_find(const char *name)
{
    assert(this->level > 0);

    size_t hash_tag = symbol_str_tag(name);
    p_hlist_head p_head = this->hash + (hash_tag % hash_MOD);

    p_symbol_name p_name = symbol_find_name(p_head, name);
    if (!p_name)
        return NULL;

    assert(!p_name->p_item->is_func);
    return p_name->p_item->p_var;
}
p_symbol_func symbol_table::symbol_table_func_find(const char *name)
{
    assert(this->level > 0);

    size_t hash_tag = symbol_str_tag(name);
    p_hlist_head p_head = this->hash + (hash_tag % hash_MOD);

    p_symbol_name p_name = symbol_find_name(p_head, name);
    if (!p_name)
        return NULL;

    assert(p_name->p_item->is_func);
    return p_name->p_item->p_func;
}

static inline p_symbol_str __symbol_table_str_find(p_hlist_head p_head, const char *string)
{
    p_hlist_node p_node;
    hlist_for_each(p_node, p_head)
    {
        p_symbol_str p_str = hlist_entry(p_node, symbol_str, h_node);
        if (!strcmp(p_str->string, string))
            return p_str;
    }
    return NULL;
}
p_symbol_str symbol_table::symbol_table_str_find(const char *string)
{
    size_t hash_tag = symbol_str_tag(string);
    p_hlist_head p_head = this->string_hash + (hash_tag % hash_MOD);

    return __symbol_table_str_find(p_head, string);
}
p_symbol_str symbol_table::symbol_table_str_add(const char *string)
{
    size_t hash_tag = symbol_str_tag(string);
    p_hlist_head p_head = this->string_hash + (hash_tag % hash_MOD);

    assert(!__symbol_table_str_find(p_head, string));

    p_symbol_str p_str = new symbol_str(string);
    hlist_node_add(p_head, &p_str->h_node);
    return p_str;
}
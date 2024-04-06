#include <frontend/use.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>

p_program frontend_trans(const char *input, const char *output)
{
    yyscan_t scanner;
    p_syntax_info p_info = new syntax_info(input, output);
    yylex_init_extra(p_info, &scanner);
    if (input)
    {
        yyrestart(fopen(input, "r"), scanner);
    }

    yyparse(scanner);
    p_program p_program = p_info->_p_program;
    delete (p_info);

    yylex_destroy(scanner);
    return p_program;
}
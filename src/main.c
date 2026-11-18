#include <stdio.h>
#include "parser.h"
#include "semantic.h"


int main(void) {
    ASTNode * program = parse_program(stdin, NULL);
    semantic_analyze(program);

    free_ast(program);

    return 0;
}

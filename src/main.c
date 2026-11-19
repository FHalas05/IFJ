#include "generator.h"
#include "parser.h"
#include "semantic.h"
#include <stdio.h>

int main(void) {
    ASTNode *program = parse_program(stdin, NULL);
    semantic_analyze(program);
    int result = generator(program);

    free_ast(program);

    return result;
}

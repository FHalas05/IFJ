//
// Created by Filip on 04/10/2026.
//

#include <stdio.h>
#include <stdbool.h>

#include "sym_table.h"
#include "token.h"
#include "scanner.h"

#include <stdlib.h>


bool is_number(char) {
    return true;
}

Token FSM_comments(FILE* file, SymTable sym_table, TokenType last_token) {

}

Token scanner(FILE* file, SymTable sym_table, TokenType last_token) {

    int c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '@') {
            char str[5] = "main";
            for (int i = 0; i < 4; i++) {
                if (fgetc(file) != str[i]) {
                    fprintf(stderr, "ERROR 1");
                    exit(1);
                }
            }
            Token token = {AT_MAIN, NULL};
            return token;
        }

        if (c == '/') {
            Token token;
            if ((token = FSM_comments(file, sym_table, last_token)) == NULL) {
                continue;
            }
            return token;
        }
    }
}

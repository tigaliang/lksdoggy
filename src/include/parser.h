/* A header file for parser.c.
 *
 * - Created by tigaliang on 20171111.
 */

#ifndef _PARSER_H
#define _PARSER_H

#include <stdio.h>

struct yylval_t {
    char *val;
};

typedef void (*handle_token_t)(int, const char *, int, struct yylval_t *);
int parse_file(const char *, handle_token_t handler);

// DO NOT use a value of zero, it's the flag of EOF
#define TOKEN_UNKNOWN       -1
#define TOKEN_SYSCALL       1
#define TOKEN_SYSCALL2      2
#define TOKEN_SYSCALL3      3

#endif // _PARSER_H
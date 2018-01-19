/* Extract C symbols with lex.
 *
 * - Created by tigaliang on 20171111.
 */

%{
#include <doggy.h>
#include <parser.h>

struct yylval_t yylval;
%}

%x comment
%x syscall_define

IDENTIFIER          ([a-zA-Z_][a-zA-Z0-9_]*)
SYSCALL_DEFINE      (SYSCALL_DEFINE([0-6])?\()
SYSCALL_DEFINE2     ((int|long)[ \n\t]+sys_)

%%
    /* comment */
"//".*\n                                    { ++yylineno; }
"/*"                                        { BEGIN(comment); }
<comment>[^*\n]*                            { ; }
<comment>"*"+[^/\n]                         { ; }
<comment>"*"+\n                             { ++yylineno; }
<comment>\n                                 { ++yylineno; }
<comment>"*/"                               { BEGIN(INITIAL); }

    /* syscall_define */
{SYSCALL_DEFINE}                            { BEGIN(syscall_define); }
{SYSCALL_DEFINE2}                           { BEGIN(syscall_define); if (strchr(yytext, '\n')) ++yylineno; }
<syscall_define>{IDENTIFIER}.               { BEGIN(INITIAL);
                                              if (yytext[yyleng - 1] == ',' || yytext[yyleng - 1] == '(' || yytext[yyleng - 1] == ')') {
                                                  yytext[yyleng - 1] = '\0';
                                                  if (strcmp(yytext, "ni_syscall")) {
                                                      yylval.val = yytext;
                                                      return TOKEN_SYSCALL2;
                                                  }
                                              }
                                            }
\nsys_{IDENTIFIER}:                         { ++yylineno; yytext[yyleng - 1] = '\0'; yylval.val = yytext + 5; return TOKEN_SYSCALL3; }

    /* others */
\n                                          { ++yylineno; }
.                                           { return TOKEN_UNKNOWN; }; /* Any character but '\n' */
%%
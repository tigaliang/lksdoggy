/* Main routine of the C symbols parser.
 *
 * - Created by tigaliang on 20171111.
 */

#include <parser.h>
#include <doggy.h>

// Extern definitions in parser.yy.c
extern int yylex(void);
extern void yyrestart (FILE *);
extern int yylineno;
extern struct yylval_t yylval;

int parse_file(const char *path, handle_token_t handler)
{
    int err = 0;
    int token;

    FILE *fp = fopen(path, "r");

    if (!fp) {
        LOGEE("fopen, path=%s\n", path);
        err = -1;
        goto out;
    }

    yylineno = 1;
    yyrestart(fp);
    while((token = yylex()))
        handler(token, path, yylineno, &yylval);

    fclose(fp);
out:
    return err;
}
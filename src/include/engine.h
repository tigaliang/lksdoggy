/* A header file for engine.c.
 *
 * - Created by tigaliang on 20171114.
 */

#ifndef _ENGINE_H
#define _ENGINE_H

#include <list.h>

struct file_pos_t {
    int file_idx;
    int lineno;
};

struct search_result_t {
    char *kdir;
    char **files;
    int pos_num;
    struct file_pos_t *pos;
};

int build_index(const char *, const char *, const char *);
int load_index(const char *);
int search_syscall(const char *, struct search_result_t *);

#endif // _ENGINE_H
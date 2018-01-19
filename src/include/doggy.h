/* A global header file.
 *
 * - Created by tigaliang on 20171111.
 */

#ifndef _DOGGY_H
#define _DOGGY_H

#include <stdio.h>
#include <stdlib.h>

#define LOGD(fmt, ...)  printf("[D/%s] " fmt, __FUNCTION__, ##__VA_ARGS__)
#define LOGE(fmt, ...) \
    fprintf(stderr, "[E/%s] " fmt, __FUNCTION__, ##__VA_ARGS__)
#define LOGEE(fmt, ...) \
    { fprintf(stderr, "[E/%s] " fmt, __FUNCTION__, ##__VA_ARGS__); perror(" "); }

#define mzalloc(size) calloc(1, size);

#endif // _DOGGY_H
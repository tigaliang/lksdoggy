/* Main routine of lksdoggy.
 *
 * - Created by tigaliang on 20171111.
 */

#include <doggy.h>
#include <engine.h>
#include <string.h>

int main(int argc, char const *argv[])
{
    struct search_result_t r;
    int i;

    if (argc > 2) {
        if (!strcmp(argv[1], "-b")) {
            build_index(argv[2], "arm", argv[2]);
        } else if (!strcmp(argv[1], "-l")) {
            load_index(argv[2]);
        } else if (!strcmp(argv[1], "-s")
                && !load_index(argv[2])
                && !search_syscall(argv[3], &r)) {
            for (i = 0; i < r.pos_num; ++i)
                LOGD("position %d: path=%s/%s, lineno=%d\n",
                    i, r.kdir, r.files[r.pos[i].file_idx], r.pos[i].lineno);
        }
    }

    return 0;
}
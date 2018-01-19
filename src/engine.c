/* Main routine of engine.
 *
 * - Created by tigaliang on 20171114.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <engine.h>
#include <doggy.h>
#include <parser.h>
#include <list.h>

#define INDEX_NAME "lksdoggy.index"
#define SYSCALLS_HASH_TABLE_SIZE 141

#define E_GOTO(msg, e, label) \
    { LOGE(msg); err = (e); goto label; }
#define EE_GOTO(msg, e, label) \
    { LOGEE(msg); err = (e); goto label; }

// -----------

/* struct for storing index */
struct syscall_t {
    int name_len;
    char *name;
    struct file_pos_t pos;
};

/* struct for loading index */
struct syscall2_t {
    char *name;
    int pos_num;
    struct file_pos_t *pos;
};

struct file_t {
    int path_len;
    char *path;
};

// -----------

static int g_kdirlen = 0;
static char *g_kdir = NULL;
static int g_files_num = 0;
static LIST(g_files_list);
static int g_syscalls_num = 0;
static LIST(g_syscalls_list);
static char **g_syscall_files = NULL;
// lists of syscall2_t
static struct list_t g_syscalls_hash_table[SYSCALLS_HASH_TABLE_SIZE] = { 0 };

// -----------

/* BKDRHash */
static int inline hash_syscall_name(const char *name)
{
    int hash = 0;
    while (*name)
        hash = hash * 31 + (*name++);
    return (hash & 0x7fffffff) % SYSCALLS_HASH_TABLE_SIZE;
}

int search_syscall(const char *name, struct search_result_t *r)
{
    int err = -1;
    int hash;
    struct list_node_t *n;
    struct syscall2_t *sc2;

    hash = hash_syscall_name(name);
    list_for_each(&g_syscalls_hash_table[hash], n) {
        sc2 = (struct syscall2_t *)n->data;
        if (!strcmp(name, sc2->name)) {
            r->kdir = g_kdir;
            r->files = g_syscall_files;
            r->pos_num = sc2->pos_num;
            r->pos = sc2->pos;
            err = 0;
            goto out;
        }
    }
    LOGD("syscall %s is not found.\n", name);

out:
    return err;
}

int load_index(const char *indexdir)
{
    int err = 0;
    char indexpath[PATH_MAX];
    int fd;
    int i, j;
    int tmp_path_len;
    char *tmp_path;
    int tmp_name_len;
    char *tmp_name;
    struct file_pos_t tmp_file_pos;
    int hash;
    struct list_node_t *n;
    struct syscall2_t *sc2;
    struct file_pos_t *new_pos;
    int flag;

    snprintf(indexpath, PATH_MAX, "%s/" INDEX_NAME, indexdir);
    fd = open(indexpath, O_RDONLY);
    if (-1 == fd)
        EE_GOTO("failed to open index file", -1, out);

    // load kdir
    if (-1 == read(fd, &g_kdirlen, sizeof(typeof(g_kdirlen))))
        EE_GOTO("read", -1, close_out);
    g_kdir = mzalloc(g_kdirlen + 1);
    if (!g_kdir)
        EE_GOTO("calloc", -1, close_out);
    if (-1 == read(fd, g_kdir, g_kdirlen))
        EE_GOTO("read 2", -1, free_out);

    // load file paths
    if (-1 == read(fd, &g_files_num, sizeof(typeof(g_files_num))))
        EE_GOTO("read 3", -1, free_out);
    i = g_files_num;
    g_syscall_files = mzalloc(g_files_num * sizeof(char *));
    if (!g_syscall_files)
        EE_GOTO("calloc 2", -1, free_out);
    while (i--) {
        if (-1 == read(fd, &tmp_path_len, sizeof(typeof(tmp_path_len))))
            EE_GOTO("read 4", -1, free_out);
        tmp_path = mzalloc(tmp_path_len + 1);
        if (!tmp_path)
            EE_GOTO("calloc 3", -1, free_out);
        if (-1 == read(fd, tmp_path, tmp_path_len))
            EE_GOTO("read 5", -1, free_out);
        g_syscall_files[g_files_num - i - 1] = tmp_path;
    }

    // load syscalls
    if (-1 == read(fd, &g_syscalls_num, sizeof(typeof(g_syscalls_num))))
        EE_GOTO("read 6", -1, free_out);
    i = g_syscalls_num;
    while (i--) {
        if (-1 == read(fd, &tmp_name_len, sizeof(typeof(tmp_name_len))))
            EE_GOTO("read 7", -1, free_out);
        tmp_name = mzalloc(tmp_name_len + 1);
        if (!tmp_name)
            EE_GOTO("calloc 4", -1, free_out);
        if (-1 == read(fd, tmp_name, tmp_name_len))
            EE_GOTO("read 8", -1, free_out);
        if (-1 == read(fd, &tmp_file_pos, sizeof(struct file_pos_t)))
            EE_GOTO("read 9", -1, free_out);

        hash = hash_syscall_name(tmp_name);
        flag = 0;
        list_for_each(&g_syscalls_hash_table[hash], n) {
            sc2 = (struct syscall2_t *)n->data;
            if (strcmp(sc2->name, tmp_name))
                continue;
            flag = 1;
            new_pos = mzalloc(sizeof(struct file_pos_t) *
                    (sc2->pos_num) + 1);
            if (!new_pos)
                EE_GOTO("calloc 5", -1, free_out);
            memcpy(new_pos, sc2->pos, sizeof(struct file_pos_t) *
                    sc2->pos_num);
            new_pos[sc2->pos_num].file_idx = tmp_file_pos.file_idx;
            new_pos[sc2->pos_num].lineno = tmp_file_pos.lineno;
            sc2->pos_num += 1;
            free(sc2->pos);
            sc2->pos = new_pos;
        }
        if (flag) continue;
        n = mzalloc(sizeof(struct list_node_t));
        if (!n)
            EE_GOTO("calloc 6", -1, free_out);
        sc2 = mzalloc(sizeof(struct syscall2_t));
        if (!sc2)
            EE_GOTO("calloc 7", -1, free_out);
        sc2->name = tmp_name;
        sc2->pos_num = 1;
        sc2->pos = mzalloc(sizeof(struct syscall2_t));
        if (!sc2->pos)
            EE_GOTO("calloc 8", -1, free_out);
        sc2->pos[0].file_idx = tmp_file_pos.file_idx;
        sc2->pos[0].lineno = tmp_file_pos.lineno;
        n->data = sc2;
        list_add_tail(&g_syscalls_hash_table[hash], n);
    }

close_out:
    close(fd);
out:
    /*if (!err) {
        LOGD("succeed to load index.\n");
        for (i = 0; i < g_files_num; ++i)
            LOGD("file %d, path=%s\n", i, g_syscall_files[i]);

        for (i = 0; i < SYSCALLS_HASH_TABLE_SIZE; ++i)
            list_for_each(&g_syscalls_hash_table[i], n) {
                sc2 = (struct syscall2_t *)n->data;
                LOGD("list %d, name=%s, pos_num=%d\n",
                        i, sc2->name, sc2->pos_num);
                for (j = 0; j < sc2->pos_num; ++j)
                    LOGD("pos %d, file_idx=%d, lineno=%d\n",
                        j, sc2->pos[j].file_idx, sc2->pos[j].lineno);
            }
    }*/
    return err;
free_out:
    if (g_kdir) free(g_kdir);
    if (g_syscall_files) {
        i = g_syscalls_num;
        while (i--)
            if (g_syscall_files[i]) free(g_syscall_files[i]);
        free(g_syscall_files);
    }
    for (i = 0; i < SYSCALLS_HASH_TABLE_SIZE; ++i) {
        while ((n = list_del_next(&g_syscalls_hash_table[i], NULL))) {
            sc2 = (struct syscall2_t *)n->data;
            free(sc2->name);
            free(sc2->pos);
            free(sc2);
            free(n);
        }
    }
    goto out;
}

static void handle_syscall(const char *name, const char *file, int lineno)
{
    struct list_node_t *n;
    struct file_t *f;
    int file_path_len;
    struct syscall_t *sc;
    int sc_name_len;

    LOGD("sys_call found, file=%s, line=%d, name=%s\n", file, lineno, name);
    ++g_syscalls_num;

    if (list_empty(&g_files_list) || strcmp(file,
                ((struct file_t *)g_files_list.tail->data)->path)) {
        n = mzalloc(sizeof(struct list_node_t));
        if (!n) {
            LOGEE("calloc");
            exit(EXIT_FAILURE);
        }
        f = mzalloc(sizeof(struct file_t));
        if (!f) {
            LOGEE("calloc 2");
            exit(EXIT_FAILURE);
        }
        n->data = f;
        file_path_len = strlen(file);
        f->path_len = file_path_len;
        f->path = mzalloc(file_path_len + 1);
        if (!f->path) {
            LOGEE("calloc 3");
            exit(EXIT_FAILURE);
        }
        memcpy(f->path, file, file_path_len);
        list_add_tail(&g_files_list, n);
        ++g_files_num;
    }

    n = mzalloc(sizeof(struct list_node_t));
    if (!n) {
        LOGEE("calloc 4");
        exit(EXIT_FAILURE);
    }
    sc = mzalloc(sizeof(struct syscall_t));
    if (!sc) {
        LOGEE("calloc 5");
        exit(EXIT_FAILURE);
    }
    n->data = sc;
    sc_name_len = strlen(name);
    sc->name_len = sc_name_len;
    sc->name = mzalloc(sc_name_len + 1);
    if (!sc->name) {
        LOGEE("calloc 6");
        exit(EXIT_FAILURE);
    }
    memcpy(sc->name, name, sc_name_len);
    sc->pos.file_idx = g_files_num - 1;
    sc->pos.lineno = lineno;
    list_add_tail(&g_syscalls_list, n);
}

static void handle_token(int token, const char *file, int lineno,
        struct yylval_t *yylval)
{
    char *suffix;

    switch(token) {
    case TOKEN_SYSCALL:
    case TOKEN_SYSCALL2:
    case TOKEN_SYSCALL3:
        suffix = strrchr(file, '.');
        if (!suffix || strcmp(suffix, token == TOKEN_SYSCALL3 ? ".S" : ".c"))
            break;
        handle_syscall(yylval->val, file + (g_kdirlen + 2), lineno);
        break;
    default:
        break;
    }
}

static int enqueue_new_path(struct queue_t *q, const char *path)
{
    int err = 0;
    struct list_node_t *n;
    size_t len;

    n = mzalloc(sizeof(struct list_node_t));
    if (!n) EE_GOTO("calloc", -1, out);

    len = strlen(path);
    n->data = mzalloc(len + 1);
    if (!n->data) {
        free(n);
        EE_GOTO("calloc 2", -1, out);
    }
    memcpy(n->data, path, len);

    queue_push(q, n);

out:
    return err;
}

int build_index(const char *kdir, const char *arch, const char *indexdir)
{
    int err = 0;
    struct stat st;
    DIR *dir;
    struct dirent *de;
    char parent[PATH_MAX];
    char path[PATH_MAX];
    char indexpath[PATH_MAX];
    char *p;
    QUEUE(q);
    struct list_node_t *n;
    struct file_t *f;
    struct syscall_t *sc;
    int rootdir = 1;
    int nfiles = 0;
    int nskips = 0;
    int fd;
    int i;

    g_kdirlen = strlen(kdir);

    if (enqueue_new_path(&q, ""))
        E_GOTO("failed to push new path\n", -1, out);

    while (!queue_empty(&q)) {
        n = queue_pop(&q);
        snprintf(parent, PATH_MAX, "%s/%s", kdir, (char *)n->data);

        dir = opendir(parent);
        if (!dir) {
            LOGEE("opendir, path=%s\n", parent);
            err = -1;
            goto _break;
        }

        while ((de = readdir(dir))) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                continue;
            snprintf(path, PATH_MAX, "%s/%s", parent, de->d_name);

            if (lstat(path, &st)) {
                LOGEE("lstat, path=%s\n", path);
                err = -1;
                goto _break;
            }

            if (S_ISREG(st.st_mode)) {
                p = strrchr(path, '.');
                if (p && (!strcmp(p, ".c" ) || !strcmp(p, ".S"))) {
                    // LOGD("parsing file %s\n", path);
                    err = parse_file(path, handle_token);
                    if (err) {
                        LOGE("failed to parse %s\n", path);
                        goto _break;
                    }
                    ++nfiles;
                } else
                    ++nskips;
            } else if (S_ISDIR(st.st_mode)) {
                if (!strcmp((char *)n->data, "/arch") && arch
                        && strcmp(de->d_name, arch))
                    LOGD("skip unwanted arch: %s\n", de->d_name);
                else if (rootdir && (!strcmp(de->d_name, "Documentation")
                        || !strcmp(de->d_name, "scripts")
                        || !strcmp(de->d_name, "drivers"))) {
                    LOGD("skip unwanted directory: %s\n", de->d_name);
                } else {
                    snprintf(path, PATH_MAX, "%s/%s",
                        (char *)n->data, de->d_name);
                    enqueue_new_path(&q, path);
                }
            } else
                LOGD("skip path %s of mode 0x%08x\n", path, st.st_mode);
        }
_break:
        rootdir = 0;
        if (dir)
            closedir(dir);
        free(n->data);
        free(n);

        if (err) {
            while (!queue_empty(&q)) {
                n = queue_pop(&q);
                free(n->data);
                free(n);
            }
            break;
        }
    }

    snprintf(indexpath, PATH_MAX, "%s/%s", indexdir, INDEX_NAME);
    if (!access(indexpath, F_OK) && unlink(indexpath))
        EE_GOTO("failed to remove old index", -1, out);

    fd = open(indexpath, O_WRONLY | O_CREAT);
    if (-1 == fd)
        EE_GOTO("failed to open index file for writing.", -1, out);
    if (-1 == write(fd, &g_kdirlen, sizeof(typeof(g_kdirlen))))
        EE_GOTO("write", -1, close_out);
    if (-1 == write(fd, kdir, g_kdirlen))
        EE_GOTO("write 2", -1, close_out);
    if (-1 == write(fd, &g_files_num, sizeof(typeof(g_files_num))))
        EE_GOTO("write 3", -1, close_out);
    list_for_each(&g_files_list, n) {
        f = (struct file_t *)n->data;
        if (-1 == write(fd, &f->path_len, sizeof(typeof(f->path_len))))
            EE_GOTO("write 4", -1, close_out);
        if (-1 == write(fd, f->path, f->path_len))
            EE_GOTO("write 5", -1, close_out);
    }
    if (-1 == write(fd, &g_syscalls_num, sizeof(typeof(g_syscalls_num))))
        EE_GOTO("write 6", -1, close_out);
    list_for_each(&g_syscalls_list, n) {
        sc = (struct syscall_t *)n->data;
        if (-1 == write(fd, &sc->name_len, sizeof(typeof(sc->name_len))))
            EE_GOTO("write 7", -1, close_out);
        if (-1 == write(fd, sc->name, sc->name_len))
            EE_GOTO("write 8", -1, close_out);
        if (-1 == write(fd, &sc->pos, sizeof(struct file_pos_t)))
            EE_GOTO("write 9", -1, close_out);
    }
    fchmod(fd, 0644);
close_out:
    close(fd);
out:
    i = 0;
    while ((n = list_del_next(&g_files_list, NULL))) {
        LOGD("file %d, path_len=%d, path=%s\n", i++,
            ((struct file_t *)n->data)->path_len,
            ((struct file_t *)n->data)->path);
        free(((struct file_t *)n->data)->path);
        free(n->data);
        free(n);
    }
    i = 0;
    while ((n = list_del_next(&g_syscalls_list, NULL))) {
        LOGD("syscall %d, name_len=%d, name=%s, file_idx=%d, lineno=%d\n",
            i++,
            ((struct syscall_t *)n->data)->name_len,
            ((struct syscall_t *)n->data)->name,
            ((struct syscall_t *)n->data)->pos.file_idx,
            ((struct syscall_t *)n->data)->pos.lineno);
        free(((struct syscall_t *)n->data)->name);
        free(n->data);
        free(n);
    }
    if (err)
        unlink(indexpath);
    LOGD("%d files are parsed, %d are skipped.\n", nfiles, nskips);
    LOGD("%d syscalls are found in %d files.\n", g_syscalls_num, g_files_num);
    return err;
}

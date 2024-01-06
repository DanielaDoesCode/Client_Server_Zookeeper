#ifndef _ZOO_CLIENT_H
#define _ZOO_CLIENT_H
#include <zookeeper/zookeeper.h>
typedef struct String_vector zoo_string;

static void watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);
char* get_host_address();
char* get_successor(zoo_string* children_list);
char* get_anteccessor(zoo_string* children_list);
static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);

#endif
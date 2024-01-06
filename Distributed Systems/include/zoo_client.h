#ifndef _ZOO_CLIENT_H
#define _ZOO_CLIENT_H
#include <zookeeper/zookeeper.h>
typedef struct String_vector zoo_string;

char* get_highest_id_node(zoo_string* children_list);
char* get_lowest_id_node(zoo_string* children_list);
void watcher(zhandle_t *zzh, int type, int state, const char *path, void *context);
static void child_watcher(zhandle_t *wzh, int type, int state, const char* zpath, void *watcher_ctx);

#endif
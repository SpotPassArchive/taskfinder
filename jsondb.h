#ifndef _grabber_jsondb_h
#define _grabber_jsondb_h

#include <jansson.h>
#include <string.h>

typedef struct jsondb_app_ids {
	size_t count;
	const char *app_ids[];
} jsondb_app_ids;

int jsondb_read(json_t **db, char *filename);
int jsondb_write(json_t *db, char *filename);

int jsondb_add_task(json_t *db, const char *app_id, char *taskname);
int jsondb_task_exists(json_t *db, const char *app_id, char *taskname);
int jsondb_list_app_ids(json_t *db, jsondb_app_ids **out_ids);

#endif

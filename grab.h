#include <curl/curl.h>
#include <curl/multi.h>
#include <curl/easy.h>
#include <stdlib.h>

#include <jsondb.h>
#include "cert.h"

typedef enum grab_platform {
	GRAB_CTR = 0,
	GRAB_WUP
} grab_platform;

typedef enum grab_task_type {
	GRAB_TYPE_NORMAL,
	GRAB_TYPE_NUMBERED_BEGIN,
	GRAB_TYPE_NUMBERED_EXTEND
} grab_task_type;

typedef struct grab_task {
	CURL *handle; // using CURLOPT_DEBUGDATA to store a pointer to this
	grab_platform platform;
	const char *app_id;
	char *taskname;
	int numbered_value;
	int numbered_width;
	char url[256];
} grab_task;

typedef struct grab_context {
	grab_task* tasks;
	json_t *db;
	CURLM *multi;
	size_t task_count;
} grab_context;

int grab_init(grab_context *ctx, json_t *db, grab_platform platform, size_t task_count);
void grab_cleanup(grab_context *ctx);
int grab_run_for_task(grab_context *ctx, jsondb_app_ids *app_ids, char *task);

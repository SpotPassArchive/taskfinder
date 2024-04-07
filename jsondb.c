#include <jsondb.h>

int jsondb_read(json_t **db, char *filename) {
	json_error_t err;
	json_t *j = json_load_file(filename, 0, &err);
	if (!j) {
		fprintf(stderr, "JSON failure: %s\n", err.text);
		return 0;
	}

	// i will not be looping through the entire json every single time!!!!
	json_t *sane = json_object();
	size_t index;
	json_t *value;

	json_array_foreach(j, index, value) {
		json_object_set(sane, json_string_value(json_object_get(value, "app_id")), value);
	}

	json_decref(j);
	*db = sane;
	return 1;
}

int jsondb_list_app_ids(json_t *db, jsondb_app_ids **out_ids) {
	size_t n = json_object_size(db);
	jsondb_app_ids *ids = (jsondb_app_ids *)malloc(sizeof(const char *) * n + sizeof(size_t));
	if (!ids) return 0;

	json_t *value;
	const char *key;
	size_t idx = 0;
	json_object_foreach(db, key, value) {
		ids->app_ids[idx++] = key;
	}

	ids->count = n;
	*out_ids = ids;
	return 1;
}

int jsondb_write(json_t *db, char *filename) {
	json_t *out = json_array();

	const char *key;
	json_t *value;

	json_object_foreach(db, key, value) {
		json_t *nkey = json_string(key);
		json_object_set(value, "app_id", nkey);
		json_array_append(out, value);
		json_decref(nkey);
	}

	int r = json_dump_file(out, filename, JSON_INDENT(4));
	json_decref(out);
	return r == 0;
}

static json_t *resolve_task_array(json_t *db, const char *app_id) {
	json_t *app = json_object_get(db, app_id);
	if (!app) return NULL;
	return json_object_get(app, "tasks");
}

int jsondb_task_exists(json_t *db, const char *app_id, char *taskname) {
	size_t idx;
	json_t *value;

	json_t *taskarr = resolve_task_array(db, app_id);
	if (!taskarr) return 0;

	json_array_foreach(taskarr, idx, value) {
		const char *val = json_string_value(value);
		if (strcmp(val, taskname) == 0)
			return 1;
	}
	return 0;
}

// this does not check whether or not the input task already exists
int jsondb_add_task(json_t *db, const char *app_id, char *taskname) {
	json_t *taskarr = resolve_task_array(db, app_id);
	if (!taskarr) return 0;

	json_t *nval = json_string(taskname);
	json_array_append(taskarr, nval);
	json_decref(nval);
	return 1;
}

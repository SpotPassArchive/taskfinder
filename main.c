#include <stdio.h>

#include <grab.h>
#include <jsondb.h>

int main(int argc, char **argv) {
	curl_global_init(CURL_GLOBAL_ALL);
	json_t *db;

	if (argc != 4) {
		printf("Usage: %s <json db> <platform: ctr|wup> <task name>\n", argv[0]);
		return 1;
	}

	if (strlen(argv[3]) > 7) {
		fprintf(stderr, "task names can be at most 7 characters long\n");
		return 1;
	}

	int platform_state =
		strcmp(argv[2], "ctr") == 0 ?
			GRAB_CTR :
			strcmp(argv[2], "wup") == 0 ?
				GRAB_WUP :
				-1;

	if (platform_state == -1) {
		fprintf(stderr, "invalid platform\n");
		return 1;
	}

	if (!jsondb_read(&db, argv[1])) {
		fprintf(stderr, "failed loading json\n");
		return 1;
	}

	jsondb_app_ids *ids;

	jsondb_list_app_ids(db, &ids);

	grab_context gctx;
	grab_init(&gctx, db, platform_state, 256);
	grab_run_for_task(&gctx, ids, argv[3]);
	grab_cleanup(&gctx);
	free(ids);

	curl_global_cleanup();

//	jsondb_write(db, argv[1]);

	json_decref(db);

	return 0;
}

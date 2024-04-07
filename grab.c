#include <grab.h>

#define NPFL_BASE "https://npfl.c.app.nintendowifi.net/p01/filelist/%s/%s?c=%s&l=%s"
#define NPTS_BASE "https://npts.app.nintendo.net/p01/tasksheet/1/%s/%s?c=%s&l=%s"

static const char *countries[133] = { 
	"GB", "US", "IT", "NL", "DE", "CA", "FR", "HU", "CR", "AU",
	"BR", "RO", "CL", "MX", "RU", "ES", "JP", "CZ", "PT", "MT",
	"AR", "SE", "PL", "IE", "BE", "HT", "NO", "FI", "GR", "BO",
	"AT", "VE", "PA", "PE", "GF", "SA", "CO", "LT", "NA", "CH",
	"CY", "RS", "KY", "GP", "DK", "KR", "LU", "SV", "VA", "GT",
	"SK", "HR", "ZA", "DO", "UY", "LV", "HN", "JM", "TR", "IN",
	"ER", "AW", "NZ", "EC", "TW", "EE", "CN", "SI", "AI", "BG",
	"NI", "IS", "MQ", "BZ", "BA", "MY", "AZ", "ZW", "AL", "IM",
	"VG", "VI", "BM", "GY", "SR", "MS", "TC", "BB", "TT", "AG",
	"BS", "DM", "GD", "AN", "PY", "KN", "LC", "VC", "BW", "LS",
	"LI", "MK", "ME", "MZ", "SZ", "ZM", "MR", "ML", "NE", "TD",
	"SD", "DJ", "SO", "AD", "GI", "JE", "MC", "HK", "MO", "ID",
	"SG", "TH", "PH", "AE", "EG", "OM", "QA", "KW", "SY", "BH",
	"JO", "SM", "GG"
};
static const size_t country_count = 133;

static const char *languages[12] = {
	"en", "it", "de", "fr",
	"es", "pt", "ru", "ja",
	"nl", "ko", "zh", "tw"
};
static const size_t language_count = 12;

int grab_init(grab_context *ctx, json_t *db, grab_platform platform, size_t task_count) {
	grab_task *tasks = (grab_task *)malloc(sizeof(grab_task) * task_count);
	if (!tasks) {
		fprintf(stderr, "failed allocating memory for grab tasks\n");
		return 0;
	}
	memset(tasks, 0, sizeof(grab_task) * task_count);
	CURLM *multi = curl_multi_init();
	if (!multi) {
		free(tasks);
		fprintf(stderr, "failed getting curl multi handle\n");
		return 0;
	}
	curl_multi_setopt(multi, CURLMOPT_MAX_TOTAL_CONNECTIONS, 500);

	for (size_t i = 0; i < task_count; i++) {
		tasks[i].platform = platform;
		tasks[i].handle = curl_easy_init();
		curl_easy_setopt(tasks[i].handle, CURLOPT_NOBODY, 1); /* use HEAD, not GET */
		curl_easy_setopt(tasks[i].handle, CURLOPT_SSLKEY_BLOB, &wiiu_crt_key);
		curl_easy_setopt(tasks[i].handle, CURLOPT_SSLCERT_BLOB, &wiiu_crt);
		curl_easy_setopt(tasks[i].handle, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(tasks[i].handle, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(tasks[i].handle, CURLOPT_PRIVATE, &tasks[i]);
	}

	ctx->tasks = tasks;
	ctx->multi = multi;
	ctx->task_count = task_count;
	ctx->db = db;
	return 1;
}

static void reload_first(grab_context *ctx, size_t n) {
	for (size_t i = 0; i < n; i++) {
		curl_multi_add_handle(ctx->multi, ctx->tasks[i].handle);
	}
}

void grab_cleanup(grab_context *ctx) {
	for (size_t i = 0; i < ctx->task_count; i++) {
		curl_multi_remove_handle(ctx->multi, ctx->tasks[i].handle);
		curl_easy_cleanup(ctx->tasks[i].handle);
	}
	curl_multi_cleanup(ctx->multi);
	free(ctx->tasks);
}

static int grab_execute_batch(grab_context *ctx) {
	int running = 1;
	int found = 0;
	while (running) {
		CURLMcode mc = curl_multi_perform(ctx->multi, &running);

		if (mc == CURLM_OK && running)
			mc = curl_multi_poll(ctx->multi, NULL, 0, 1000, NULL);

		if (mc != CURLM_OK) {
			fprintf(stderr, "curl error: %s\n", curl_multi_strerror(mc));
			return 0;
		}

		struct CURLMsg *msg = NULL;

		int in_queue;
		while ((msg = curl_multi_info_read(ctx->multi, &in_queue))) {
			if (msg->msg == CURLMSG_DONE) {
				grab_task *task;
				curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, (char **)&task);
				if (msg->data.result != CURLE_OK) {
					printf("[b] %s: %s\n", task->url, curl_easy_strerror(msg->data.result));
				} else {
					long status;
					curl_easy_getinfo(msg->easy_handle, CURLINFO_HTTP_CODE, &status);
					if (status == 200) {
						found = 1;
					}
					curl_multi_remove_handle(ctx->multi, msg->easy_handle);
				}
			}
		}
	}
	
	return found ? 2 : 1;
}

int grab_run_for_task(grab_context *ctx, jsondb_app_ids *app_ids, char *task) {
	for (size_t appid_idx = 0; appid_idx < app_ids->count; appid_idx++) {
		size_t in_batch = 0;

		if (jsondb_task_exists(ctx->db, app_ids->app_ids[appid_idx], task)) {
			continue;
		}

		size_t prog_max = country_count * language_count;
		size_t prog_cur = 0;

		for (size_t country_idx = 0; country_idx < country_count; country_idx++) {
			for (size_t lang_idx = 0; lang_idx < language_count; lang_idx++) {
				// execute a batch
				if (in_batch == ctx->task_count) {
exec_batch:
					size_t batch_size = in_batch;
					reload_first(ctx, in_batch);
					int r = grab_execute_batch(ctx);
					if (r == 2) {
						printf("%s->%s\n", app_ids->app_ids[appid_idx], task);
						goto next_appid;
					}
					prog_cur += in_batch;
					in_batch = 0;
					fflush(stdout);
					if (batch_size != ctx->task_count)
						goto next_appid;
				}
				snprintf(ctx->tasks[in_batch].url, 256, ctx->tasks[in_batch].platform == GRAB_CTR ? NPFL_BASE : NPTS_BASE, app_ids->app_ids[appid_idx], task, countries[country_idx], languages[lang_idx]);
				curl_easy_setopt(ctx->tasks[in_batch].handle, CURLOPT_URL, ctx->tasks[in_batch].url);
				in_batch++;
			}
		}
		if (in_batch) {
			goto exec_batch;
		}
next_appid:
	}
}

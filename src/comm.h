#ifndef _COMM_H
#define _COMM_H

typedef void (*CommJsReadyCallback)(void *data);

void comm_query_tasklists();
void comm_query_tasks(int);
void comm_query_task_details(int, int);
void comm_update_task_status(int, int, bool);
void comm_create_task(int, char *, char *);

void comm_on_js_ready(CommJsReadyCallback callback, void *data);
bool comm_is_available();
bool comm_is_available_silent();

void comm_init();
void comm_deinit();

void comm_retrieve_tokens();

#endif


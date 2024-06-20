
#include <pebble.h>
#include "comm.h"
#include "misc.h"
#include "consts.h"
#include "tasklists.h"
#include "tasks.h"
#include "taskinfo.h"
#include "statusbar.h"
#include "options.h"

static bool comm_js_ready = false;
static CommJsReadyCallback comm_js_ready_cb;
static void *comm_js_ready_cb_data;
// static int comm_array_size = -1;

static bool comm_is_bluetooth_available()
{
    if (!bluetooth_connection_service_peek())
    {
        sb_show("No bluetooth connection!");
        return false;
    }
    return true;
}

bool comm_is_available()
{
    if (!comm_is_bluetooth_available())
        return false;
    if (!comm_js_ready)
    {
        sb_show("JS not available, please try again later");
        return false;
    }
    return true;
}

bool comm_is_available_silent()
{
    return bluetooth_connection_service_peek() && comm_js_ready;
}

void comm_query_tasklists_cb(void *arg)
{
    comm_query_tasklists();
}

void comm_query_tasklists()
{
    if (!comm_js_ready)
    {
        comm_js_ready_cb = comm_query_tasklists_cb;
        sb_show("Waiting for JS...");
        comm_is_bluetooth_available(); // check bluetooth connection and show message if needed
        return;
    }
    if (!comm_is_available())
        return;
    sb_show("Connecting...");
    LOG("Querying tasklists");
    DictionaryIterator *iter;
    Tuplet code = TupletInteger(KEY_CODE, CODE_GET);
    Tuplet scope = TupletInteger(KEY_SCOPE, SCOPE_LISTS);

    app_message_outbox_begin(&iter);
    dict_write_tuplet(iter, &code);
    dict_write_tuplet(iter, &scope);
    app_message_outbox_send();
}

void comm_query_tasks_cb(void *arg)
{
    comm_query_tasks((int)arg);
}

void comm_query_tasks(int listId)
{
    if (!comm_js_ready)
    {
        comm_js_ready_cb = comm_query_tasks_cb;
        comm_js_ready_cb_data = (void *)listId;
        comm_is_available(); // show message if needed
        return;
    }
    if (!comm_is_available())
        return;
    sb_show("Connecting...");
    LOG("Querying tasks for listId=%d", listId);
    DictionaryIterator *iter;
    Tuplet code = TupletInteger(KEY_CODE, CODE_GET);
    Tuplet scope = TupletInteger(KEY_SCOPE, SCOPE_TASKS);
    Tuplet tListId = TupletInteger(KEY_LISTID, listId);

    app_message_outbox_begin(&iter);
    dict_write_tuplet(iter, &code);
    dict_write_tuplet(iter, &scope);
    dict_write_tuplet(iter, &tListId);
    app_message_outbox_send();
}

void comm_query_task_details(int listId, int taskId)
{
    LOG("Querying task details for %d, %d (not implemented)", listId, taskId);
}

void comm_update_task_status(int listId, int taskId, bool newStatus)
{
    if (!comm_is_available())
        return;
    LOG("Updating status for task %d->%d to %d (NI)", listId, taskId, newStatus);
    sb_show("Updating...");
    DictionaryIterator *iter;
    Tuplet code = TupletInteger(KEY_CODE, CODE_UPDATE);
    Tuplet scope = TupletInteger(KEY_SCOPE, SCOPE_TASK);
    Tuplet tListId = TupletInteger(KEY_LISTID, listId);
    Tuplet tTaskId = TupletInteger(KEY_TASKID, taskId);
    Tuplet tIsDone = TupletInteger(KEY_ISDONE, newStatus);

    app_message_outbox_begin(&iter);
    dict_write_tuplet(iter, &code);
    dict_write_tuplet(iter, &scope);
    dict_write_tuplet(iter, &tListId);
    dict_write_tuplet(iter, &tTaskId);
    dict_write_tuplet(iter, &tIsDone);
    app_message_outbox_send();
}

void comm_create_task(int listId, char *title, char *notes)
{
    if (!comm_is_available())
    {
        // TODO: alert?
        return;
    }
    LOG("Creating new task with title %s in list %d", title, listId);
    sb_show("Creating...");
    DictionaryIterator *iter;
    Tuplet code = TupletInteger(KEY_CODE, CODE_POST);
    Tuplet scope = TupletInteger(KEY_SCOPE, SCOPE_TASK);
    Tuplet tListId = TupletInteger(KEY_LISTID, listId);
    Tuplet tTitle = TupletCString(KEY_TITLE, title);

    app_message_outbox_begin(&iter);
    dict_write_tuplet(iter, &code);
    dict_write_tuplet(iter, &scope);
    dict_write_tuplet(iter, &tListId);
    dict_write_tuplet(iter, &tTitle);
    if (notes)
    {
        Tuplet tNotes = TupletCString(KEY_NOTES, notes);
        dict_write_tuplet(iter, &tNotes);
    }
    app_message_outbox_send();
}


void comm_retrieve_tokens()
{
    // Load tokens from persistent storage
    char szAccessToken[64] = "";
    char szRefreshToken[64] = "";

    // Attempt to load access token
    persist_read_string(KEY_ACCESS_TOKEN, szAccessToken, sizeof(szAccessToken));
    if (strlen(szAccessToken) > 0)
    {
        // Use szAccessToken now (e.g., send it to JS)
        LOG("Loaded access token: %s", szAccessToken);

        // Send access token to JS
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);
        Tuplet code = TupletInteger(KEY_CODE, CODE_RETRIEVE_TOKEN);
        Tuplet tAccessToken = TupletCString(KEY_ACCESS_TOKEN, szAccessToken);
        dict_write_tuplet(iter, &code);
        dict_write_tuplet(iter, &tAccessToken);
        app_message_outbox_send();
    }
    else
    {
        // Handle empty access token (e.g., log or request new token)
        LOG("No access token found in storage");
    }

    // Attempt to load refresh token
    persist_read_string(KEY_REFRESH_TOKEN, szRefreshToken, sizeof(szRefreshToken));
    if (strlen(szRefreshToken) > 0)
    {
        // Use szRefreshToken now (e.g., store it for later use)
        LOG("Loaded refresh token: %s", szRefreshToken);

        // Send refresh token to JS
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);
        Tuplet code = TupletInteger(KEY_CODE, CODE_RETRIEVE_TOKEN);
        Tuplet tRefreshToken = TupletCString(KEY_REFRESH_TOKEN, szRefreshToken);
        dict_write_tuplet(iter, &code);
        dict_write_tuplet(iter, &tRefreshToken);
        app_message_outbox_send();
    }
    else
    {
        // Handle empty refresh token (e.g., log or request new one)
        LOG("No refresh token found in storage");
    }
}



static void comm_in_received_handler(DictionaryIterator *iter, void *context)
{
    Tuple *tCode, *tMessage, *tScope;

    LOG("Used: %d, free: %d", heap_bytes_used(), heap_bytes_free());

    tCode = dict_find(iter, KEY_CODE);
    assert(tCode, "Message without code");
    int code = (int)tCode->value->int32;
    LOG("Message code: %d", code);

    if (code == CODE_ERROR)
    {
        tMessage = dict_find(iter, KEY_ERROR);
        char *message = "Unknown error";
        if (tMessage && tMessage->type == TUPLE_CSTRING)
            message = tMessage->value->cstring;
        LOG("Error received: %s", message);
        sb_show(message);
        return;
    }
    else if (code == CODE_SAVE_TOKEN)
    { // JS wants to save token
        LOG("Saving tokens");
        Tuple *tAccessToken = dict_find(iter, KEY_ACCESS_TOKEN);
        if (tAccessToken && tAccessToken->type == TUPLE_CSTRING)
            persist_write_string(KEY_ACCESS_TOKEN, tAccessToken->value->cstring); // use the same key for storage as for appMessage
        else                                                                      // if no token was passed, assume logout - delete saved token
            persist_delete(KEY_ACCESS_TOKEN);

        Tuple *tRefreshToken = dict_find(iter, KEY_REFRESH_TOKEN);
        if (tRefreshToken && tRefreshToken->type == TUPLE_CSTRING)
            persist_write_string(KEY_REFRESH_TOKEN, tAccessToken->value->cstring);
        else
            persist_delete(KEY_REFRESH_TOKEN);

        return;
    }
    else if (code == CODE_RETRIEVE_TOKEN)
    { // JS requires saved token
        LOG("Retrieving tokens");
        comm_retrieve_tokens();
        return;
    }
    else if (code == CODE_SET_OPTION)
    {
        LOG("Updating option");
        Tuple *tOptionId = dict_find(iter, KEY_OPTION_ID);
        Tuple *tOptionVal = dict_find(iter, KEY_OPTION_VALUE);
        options_update(tOptionId->value->int32, tOptionVal->value->int32);
        return;
    }
    else if (code == CODE_READY)
    { // JS just loaded
        comm_js_ready = true;
        if (comm_js_ready_cb)
        {
            LOG("JS Ready Callback awaiting, calling");
            comm_js_ready_cb(comm_js_ready_cb_data);
        }
        return;
    }

    tScope = dict_find(iter, KEY_SCOPE);
    assert(tScope, "No scope!");
    int scope = (int)tScope->value->int32;
    LOG("Message scope: %d", scope);

    if (scope == SCOPE_LISTS)
    {
        // this might be legal if we auto-switched to the only list
        // assert(tl_is_active(), "Ignoring TaskLists-related message because that list is inactive");
    }
    else if (scope == SCOPE_TASKS)
    {
        assert(ts_is_active(), "Ignoring Tasks-related message because tasks is inactive");
    }
    else if (scope == SCOPE_TASK)
    {
        assert(ti_is_active(), "Ignoring Task-related message because that task is inactive");
    }
    else
    {
        assert(0, "Unknown scope: %d", scope);
    }
    LOG("Message in with all checks done");
}

static void comm_in_dropped_handler(AppMessageResult reason, void *context)
{
    LOG("comm_in_dropped_handler called %d", reason);
}

static void comm_out_sent_handler(DictionaryIterator *iter, void *context)
{
    LOG("comm_out_sent_handler called");
}

static void comm_out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context)
{
    LOG("comm_out_failed_handler called");
    sb_show("Connection error!");
}

void comm_init()
{
    LOG("Initializing comm");
    app_message_register_inbox_received(comm_in_received_handler);
    app_message_register_inbox_dropped(comm_in_dropped_handler);
    app_message_register_outbox_sent(comm_out_sent_handler);
    app_message_register_outbox_failed(comm_out_failed_handler);

    const int inbox = 64, outbox = 64;
    app_message_open(inbox, outbox);
}

void comm_deinit()
{
    LOG("Deinitializing comm");
    app_message_deregister_callbacks();
}

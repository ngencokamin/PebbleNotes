// Global variables for tokens and data
var my_service_token;
var my_tasklists = [];
var my_tasks = {};

// Event listener for Pebble ready event
Pebble.addEventListener("ready", function (e) {
    console.log("JS is ready!");
    Pebble.sendAppMessage({ "KEY_CODE": "CODE_READY" });

    // Retrieve tokens from local storage if available
    my_service_token = {
        access_token: localStorage.getItem('access_token'),
        refresh_token: localStorage.getItem('refresh_token')
    };

    if (my_service_token.access_token) {
        console.log("Tokens loaded from local storage");
    } else {
        console.log("No tokens found in local storage");
    }
});

// Event listener for Pebble appmessage event
Pebble.addEventListener("appmessage", function (e) {
    var dict = e.payload;

    if (dict.KEY_CODE) {
        switch (dict.KEY_CODE) {
            case "CODE_GET":
                if (dict.KEY_SCOPE) {
                    switch (dict.KEY_SCOPE) {
                        case "SCOPE_LISTS":
                            // Handle getting tasklists
                            getTasklists();
                            break;
                        case "SCOPE_TASKS":
                            // Handle getting tasks for a list
                            if (dict.KEY_LISTID !== undefined) {
                                getTasks(dict.KEY_LISTID);
                            }
                            break;
                        case "SCOPE_TASK":
                            // Handle getting task details
                            if (dict.KEY_LISTID !== undefined && dict.KEY_TASKID !== undefined) {
                                getTaskDetails(dict.KEY_LISTID, dict.KEY_TASKID);
                            }
                            break;
                    }
                }
                break;

            case "CODE_UPDATE":
                if (dict.KEY_SCOPE === "SCOPE_TASK" && dict.KEY_LISTID !== undefined && dict.KEY_TASKID !== undefined && dict.KEY_ISDONE !== undefined) {
                    // Handle updating task status
                    updateTaskStatus(dict.KEY_LISTID, dict.KEY_TASKID, dict.KEY_ISDONE);
                }
                break;

            case "CODE_POST":
                if (dict.KEY_SCOPE === "SCOPE_TASK" && dict.KEY_LISTID !== undefined && dict.KEY_TITLE !== undefined) {
                    // Handle creating a new task
                    createTask(dict.KEY_LISTID, dict.KEY_TITLE, dict.KEY_NOTES);
                }
                break;

            case "CODE_RETRIEVE_TOKEN":
                // Handle token retrieval
                sendTokens();
                break;
        }
    }
});

// Function to fetch tasklists
function getTasklists() {
    console.log("Fetching tasklists...");
    // Simulate tasklists response
    var response = { tasklists: my_tasklists };

    var dict = { "KEY_CODE": "CODE_ARRAY_START", "KEY_COUNT": response.tasklists.length, "KEY_SCOPE": "SCOPE_LISTS" };
    Pebble.sendAppMessage(dict);

    response.tasklists.forEach(function (tasklist, index) {
        var itemDict = { "KEY_CODE": "CODE_ARRAY_ITEM", "KEY_SCOPE": "SCOPE_LISTS", "KEY_INDEX": index, "KEY_LISTID": tasklist.id, "KEY_TITLE": tasklist.title };
        Pebble.sendAppMessage(itemDict);
    });

    Pebble.sendAppMessage({ "KEY_CODE": "CODE_ARRAY_END", "KEY_SCOPE": "SCOPE_LISTS" });
}

// Function to fetch tasks for a given list ID
function getTasks(listID) {
    console.log("Fetching tasks for list ID: " + listID);
    // Simulate tasks response
    var response = { tasks: my_tasks[listID] };

    var dict = { "KEY_CODE": "CODE_ARRAY_START", "KEY_COUNT": response.tasks.length, "KEY_SCOPE": "SCOPE_TASKS" };
    Pebble.sendAppMessage(dict);

    response.tasks.forEach(function (task, index) {
        var itemDict = { "KEY_CODE": "CODE_ARRAY_ITEM", "KEY_SCOPE": "SCOPE_TASKS", "KEY_INDEX": index, "KEY_LISTID": listID, "KEY_TASKID": task.id, "KEY_TITLE": task.title, "KEY_ISDONE": task.isDone };
        Pebble.sendAppMessage(itemDict);
    });

    Pebble.sendAppMessage({ "KEY_CODE": "CODE_ARRAY_END", "KEY_SCOPE": "SCOPE_TASKS" });
}

// Function to fetch task details for a given list ID and task ID
function getTaskDetails(listID, taskID) {
    console.log("Fetching task details for list ID: " + listID + ", task ID: " + taskID);
    // Simulate task details response
    var task = my_tasks[listID].find(function(task) {
        return task.id === taskID;
    });

    if (task) {
        var dict = { "KEY_CODE": "CODE_ARRAY_ITEM", "KEY_SCOPE": "SCOPE_TASK", "KEY_LISTID": listID, "KEY_TASKID": taskID, "KEY_TITLE": task.title, "KEY_ISDONE": task.isDone, "KEY_NOTES": task.notes };
        Pebble.sendAppMessage(dict);
    }
}

// Function to update task status for a given list ID and task ID
function updateTaskStatus(listID, taskID, isDone) {
    console.log("Updating task status for list ID: " + listID + ", task ID: " + taskID + " to " + isDone);
    // Simulate task status update
    var task = my_tasks[listID].find(function(task) {
        return task.id === taskID;
    });
    if (task) {
        task.isDone = isDone;
    }

    // Notify watch about the update
    Pebble.sendAppMessage({ "KEY_CODE": "CODE_UPDATE", "KEY_SCOPE": "SCOPE_TASK", "KEY_LISTID": listID, "KEY_TASKID": taskID, "KEY_ISDONE": isDone });
}

// Function to create a new task for a given list ID, title, and optional notes
function createTask(listID, title, notes) {
    console.log("Creating new task with title: " + title + " in list ID: " + listID);
    // Simulate task creation
    var newTask = { id: Date.now(), title: title, isDone: false, notes: notes || "" };
    my_tasks[listID].push(newTask);

    // Notify watch about the new task
    Pebble.sendAppMessage({ "KEY_CODE": "CODE_POST", "KEY_SCOPE": "SCOPE_TASK", "KEY_LISTID": listID, "KEY_TASKID": newTask.id, "KEY_TITLE": newTask.title, "KEY_NOTES": newTask.notes });
}

// Function to send tokens to Pebble
function sendTokens() {
    console.log("Sending tokens...");
    var dict = { "KEY_CODE": "CODE_RETRIEVE_TOKEN", "KEY_ACCESS_TOKEN": my_service_token.access_token, "KEY_REFRESH_TOKEN": my_service_token.refresh_token };
    Pebble.sendAppMessage(dict);
}

// Handle the configuration page request
Pebble.addEventListener("showConfiguration", function() {
    var url = 'https://2-dot-pebble-notes-426618.uc.r.appspot.com/notes-config.html'; // Replace with your actual URL
    console.log('Opening configuration page: ' + url);
    Pebble.openURL(url);
});

// Handle the response from the configuration page
Pebble.addEventListener("webviewclosed", function(e) {
    if (e.response) {
        console.log('Configuration page returned: ' + e.response);
        var configData = JSON.parse(e.response);
        // Process the configuration data received from the settings page
        // Typically, you would send this data to your Pebble watch app
        // using Pebble.sendAppMessage({ ... });
    } else {
        console.log('Configuration cancelled');
    }
});

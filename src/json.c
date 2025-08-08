#include <stdio.h>
#include <stdlib.h>
#include "json.h"

void parse_and_print_json(const char *json_string) {
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        printf("Error parsing JSON!\n");
        return;
    }

    cJSON *device = cJSON_GetObjectItem(root, "device");
    cJSON *status = cJSON_GetObjectItem(root, "status");
    cJSON *count = cJSON_GetObjectItem(root, "count");

    if (cJSON_IsString(device) && device->valuestring) {
        printf("Device: %s\n", device->valuestring);
    }
    if (cJSON_IsString(status) && status->valuestring) {
        printf("Status: %s\n", status->valuestring);
    }
    if (cJSON_IsNumber(count)) {
        printf("Count: %d\n", count->valueint);
    }

    cJSON_SetIntValue(count, 20);
    cJSON_ReplaceItemInObject(root, "status", cJSON_CreateString("inactive"));

    char *modified_json = cJSON_Print(root);
    if (modified_json) {
        printf("Modified JSON:\n%s\n", modified_json);
        cJSON_free(modified_json);
    } else {
        printf("Failed to print modified JSON.\n");
    }

    cJSON_Delete(root);
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
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

bool json_save_to_file(const char *path, const char *json_text) {
    if (path == NULL || json_text == NULL) { return false; }
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) { return false; }
    const size_t len = strlen(json_text);
    size_t written = fwrite(json_text, 1, len, fp);
    int rc = fclose(fp);
    return (written == len) && (rc == 0);
}

bool json_load_file(const char *path, char **out_text) {
    if (path == NULL || out_text == NULL) { return false; }
    *out_text = NULL;
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) { return false; }
    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return false; }
    long sz = ftell(fp);
    if (sz < 0 || sz > 1024L * 1024L) { fclose(fp); return false; }
    if (fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); return false; }
    char *buf = (char*)malloc((size_t)sz + 1U);
    if (buf == NULL) { fclose(fp); return false; }
    size_t rd = fread(buf, 1, (size_t)sz, fp);
    fclose(fp);
    if (rd != (size_t)sz) { free(buf); return false; }
    buf[sz] = '\0';
    *out_text = buf;
    return true;
}


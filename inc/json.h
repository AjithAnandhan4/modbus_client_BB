#ifndef JSON_H
#define JSON_H

#include "cJSON.h"
#include <stdbool.h>

void parse_and_print_json(const char *json_string);

// Simple JSON file persistence helpers
bool json_save_to_file(const char *path, const char *json_text);
bool json_load_file(const char *path, char **out_text);

#endif /* JSON_H */ 


#include "json.h"

int main() {
    const char *json_string = "{\"device\":\"modbus_client\",\"status\":\"active\",\"count\":10}";
    parse_and_print_json(json_string);
    return 0;
}


#ifndef JSON_H
#define JSON_H

enum json_types {
    JSON_ARRAY,
    JSON_BOOLEAN,
    JSON_DOUBLE,
    JSON_INTEGER,
    JSON_NULL,
    JSON_OBJECT,
    JSON_STRING,
};
typedef struct json * json_t;

json_t json_create(int type, ...);
const char *json_get_error(void);

#endif


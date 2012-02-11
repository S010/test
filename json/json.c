#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "json.h"

struct json {
    int      type;
    char    *key;       /* used if this json instance is inside json object */
    json_t   next;

    union {
        json_t   head;  /* JSON_OBJECT and JSON_ARRAY are implemented as linked lists */
        int      i;     /* JSON_INTEGER (the actual json type is "number"); also used for JSON_BOOLEAN */
        double   d;     /* JSON_DOUBLE (the actual json type is "number") */
        char    *s;     /* JSON_STRING */
        /* JSON_NULL doesn't need a value */
    } val;
};

static const char *error;
const char *errors[] = {
    "memory allocation failed",
    "invalid type",
};
enum errors {
    E_MEM_ALLOC,
    E_INVALID_ARG,
    E_END,
};

static void
set_error(int e)
{
    assert(e > 0 && e < E_END);

    error = errors[e];
}

static char *
str_dup(const char *s)
{
    char    *buf;
    size_t   s_size;

    if (s == NULL)
        return NULL;

    s_size = strlen(s) + 1;
    buf = malloc(s_size);
    if (buf == NULL)
        return NULL;
    memcpy(buf, s, s_size);

    return buf;
}

const char *
json_get_error(void)
{
    return error;
}

json_t
json_create(int type, ...)
{
    json_t       json;
    va_list      ap;
    int          i_arg;
    double       d_arg;
    const char  *s_arg;

    if (type < 0 || type > JSON_STRING) {
        set_error(E_INVALID_ARG);
        return NULL;
    }

    json = calloc(1, sizeof(*json));
    if (json == NULL) {
        set_error(E_MEM_ALLOC);
        return NULL;
    }

    json->type = type;

    va_start(ap, type);
    switch (type) {
    case JSON_BOOLEAN:
        i_arg = va_arg(ap, int);
        json->val.i = i_arg ? 1 : 0;
        break;
    case JSON_DOUBLE:
        d_arg = va_arg(ap, double);
        json->val.d = d_arg;
        break;
    case JSON_INTEGER:
        i_arg = va_arg(ap, int);
        json->val.i = i_arg;
        break;
    case JSON_STRING:
        s_arg = va_arg(ap, const char *);
        json->val.s = str_dup(s_arg);
        if (s_arg != NULL && json->val.s == NULL) {
            set_error(E_MEM_ALLOC);
            free(json);
            return NULL;
        }
        break;
    }
    va_end(ap);

    return json;
}

void
json_free(json_t json)
{
    json_t   p, next;

    if (json == NULL)
        return;

    switch (json->type) {
    case JSON_STRING:
        free(json->val.s);
        break;
    case JSON_OBJECT:
    case JSON_ARRAY:
        for (p = json->val.head; p != NULL; p = next) {
            next = p->next;
            free(p->key);
            json_free(p);
        }
        break;
    }

    free(json);
}

static int
is_invalid_json(json_t json, int check_in_depth)
{
    json_t   p;

    if (json == NULL)
        return 1;

    if (json->type < 0 || json->type > JSON_STRING)
        return 1;

    if (json->type == JSON_STRING && json->val.s == NULL)
        return 1;

    if (check_in_depth && (json->type == JSON_OBJECT || json->type == JSON_ARRAY))
        for (p = json->val.head; p != NULL; p = p->next)
            if (is_invalid_json(p, check_in_depth))
                return 1;

    return 0;
}

int
json_put(json_t json, ...)
{
    va_list      ap;
    const char  *s_arg;
    json_t       j_arg;
    json_t       prev, p, next;

    if (is_invalid_json(json, 1)) {
        set_error(E_INVALID_ARG);
        return -1;
    }
    if (json->type != JSON_OBJECT && json->type != JSON_ARRAY) {
        set_error(E_INVALID_ARG);
        return -1;
    }

    va_start(ap, json);
    if (json->type == JSON_OBJECT) {
        s_arg = va_arg(ap, const char *);
        j_arg = va_arg(ap, json_t);

        if (s_arg == NULL || is_invalid_json(j_arg, 1)) {
            set_error(E_INVALID_ARG);
            va_end(ap);
            return -1;
        }

        if (j_arg->key != NULL) {
            free(j_arg->key);
            j_arg->key = NULL;
        }
        j_arg->key = str_dup(s_arg);
        if (j_arg->key == NULL) {
            set_error(E_MEM_ALLOC);
            va_end(ap);
            return -1;
        }

        prev = NULL;
        next = NULL;
        for (p = json->val.head; p != NULL; p = next) {
            prev = p;
            next = p->next;

            if (!strcmp(p->key, j_arg->key))
                break;
        }

        if (p == NULL) {
            /* in JSON_OBJECT, we don't care about the order of elements, so push in front */
            j_arg->next = json->val.head;
            json->val.head = j_arg;
        } else {
            if (prev != NULL)
                prev->next = j_arg;
            j_arg->next = next;
            json_free(p);
        }
    } else { /* JSON_ARRAY */
        j_arg = va_arg(ap, json_t);

        if (is_invalid_json(j_arg, 1)) {
            set_error(E_INVALID_ARG);
            va_end(ap);
            return -1;
        }

        if (json->val.head == NULL)
            json->val.head = j_arg;
        else {
            for (p = json->val.head; p->next != NULL; p = p->next)
                /* empty */;
            p->next = j_arg;
        }
    }
    va_end(ap);

    return 0;
}

int
json_get(json_t json, ...)
{
    return 0;
}

void
print_json(json_t json)
{
    json_t p;

    if (is_invalid_json(json, 0)) {
        printf("(invalid json)\n");
        return;
    }

    switch (json->type) {
    case JSON_OBJECT:
        printf("{\n");
        for (p = json->val.head; p != NULL; p = p->next) {
            printf("\"%s\" : ", p->key);
            print_json(p);
            printf(",\n");
        }
        printf("}\n");
        break;
    case JSON_ARRAY:
        printf("[\n");
        for (p = json->val.head; p != NULL; p = p->next) {
            print_json(p);
            printf(",\n");
        }
        printf("]\n");
        break;
    case JSON_BOOLEAN:
        printf("%s", json->val.i ? "true" : "false");
        break;
    case JSON_DOUBLE:
        printf("%f", json->val.d);
        break;
    case JSON_INTEGER:
        printf("%i", json->val.i);
        break;
    case JSON_NULL:
        printf("null");
        break;
    case JSON_STRING:
        printf("\"%s\"", json->val.s);
        break;
    }
}

int
main(int argc, char **argv)
{
    json_t object, array;

    object = json_create(JSON_OBJECT);
    array = json_create(JSON_ARRAY);

    json_put(object, "some_array", array);
    json_put(array, json_create(JSON_NULL));
    json_put(array, json_create(JSON_STRING, "qweqwe"));

    print_json(object);

    return 0;
}

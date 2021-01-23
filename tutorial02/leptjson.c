#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdio.h>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char * str, lept_type type) {
    size_t i;
    int len = strlen(str);
    EXPECT(c, str[0]);
    for(i = 0; i < len-1; i++)
        if(str[i+1] != c->json[i])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += len-1;
    v->type = type;
    return LEPT_PARSE_OK;
}

#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define ISSIGN(ch)          ((ch) == 'E' || (ch) == 'e' || (ch) == '.')

static int lept_parse_number(lept_context* c, lept_value* v) {
    char* end;
    /*
     *  看答案，应该从头向尾逐步解析，将json的数字语法通过编程实现。
     *  我这里的实现是将各种异常情况找来出来，返回异常值。这样写不利于扩展，也不利于阅读
     */
    if(c->json[0] == '+' || c->json[0] == '.' || (!ISDIGIT(c->json[0]) && c->json[0] != '-'))
        return LEPT_PARSE_INVALID_VALUE;

    if((c->json[0] == '0') && (c->json[1] != '.' && c->json[1] != 'E' && c->json[1] != 'e' && c->json[1] != '\0'))
        return LEPT_PARSE_ROOT_NOT_SINGULAR;

    for(int i=0; i<strlen(c->json); i++)
        if(ISSIGN(c->json[i]) && c->json[i+1] == '\0')
            return LEPT_PARSE_INVALID_VALUE;

    /* test big number */
    errno = 0;
    v->n = strtod(c->json, &end);
    if(errno == ERANGE && (v->n == HUGE_VALL || v->n == -HUGE_VALL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    c->json = end;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}

//
// Based on the fork by Melsoft Games: https://github.com/Melsoft-Games/defold-cjson
// Now it has only JSON encoding routines.
//

#ifdef _MSC_VER 
    //not #if defined(_WIN32) || defined(_WIN64) because we have strncasecmp in mingw
    #define strncasecmp _strnicmp
    #define strcasecmp _stricmp
#endif

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include <dmsdk/sdk.h>

#include "strbuf.h"
#include "fpconv.h"

/* Lua CJSON - JSON support for Lua
 *
 * Copyright (c) 2010-2012  Mark Pulford <mark@kyne.com.au>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Caveats:
 * - JSON "null" values are represented as lightuserdata since Lua
 *   tables cannot contain "nil". Compare with cjson.null.
 * - Invalid UTF-8 characters are not detected and will be passed
 *   untouched. If required, UTF-8 error checking should be done
 *   outside this library.
 * - Javascript comments are not part of the JSON spec, and are not
 *   currently supported.
 *
 * Note: Decoding is slower than encoding. Lua spends significant
 *       time (30%) managing tables when parsing JSON since it is
 *       difficult to know object/array sizes ahead of time.
 */

#ifndef CJSON_MODNAME
    #define CJSON_MODNAME   "cjson"
#endif

#ifndef CJSON_VERSION
    #define CJSON_VERSION   "2.1.0"
#endif

#if !defined(isinf) && (defined(USE_INTERNAL_ISINF) || defined(MISSING_ISINF))
    #define isinf(x) (!isnan(x) && isnan((x) - (x)))
#endif

#define DEFAULT_SPARSE_CONVERT 0
#define DEFAULT_SPARSE_RATIO 2
#define DEFAULT_SPARSE_SAFE 10
#define DEFAULT_ENCODE_MAX_DEPTH 1000
#define DEFAULT_DECODE_MAX_DEPTH 1000
#define DEFAULT_ENCODE_INVALID_NUMBERS 0
#define DEFAULT_DECODE_INVALID_NUMBERS 1
#define DEFAULT_ENCODE_KEEP_BUFFER 1
#define DEFAULT_ENCODE_NUMBER_PRECISION 14

#ifdef DISABLE_INVALID_NUMBERS
    #undef DEFAULT_DECODE_INVALID_NUMBERS
    #define DEFAULT_DECODE_INVALID_NUMBERS 0
#endif

typedef enum {
    T_OBJ_BEGIN,
    T_OBJ_END,
    T_ARR_BEGIN,
    T_ARR_END,
    T_STRING,
    T_NUMBER,
    T_BOOLEAN,
    T_NULL,
    T_COLON,
    T_COMMA,
    T_END,
    T_WHITESPACE,
    T_ERROR,
    T_UNKNOWN
} json_token_type_t;

static const char *json_token_type_name[] =
{
    "T_OBJ_BEGIN",
    "T_OBJ_END",
    "T_ARR_BEGIN",
    "T_ARR_END",
    "T_STRING",
    "T_NUMBER",
    "T_BOOLEAN",
    "T_NULL",
    "T_COLON",
    "T_COMMA",
    "T_END",
    "T_WHITESPACE",
    "T_ERROR",
    "T_UNKNOWN",
    NULL
};

typedef struct {
    json_token_type_t ch2token[256];
    char escape2char[256];
    strbuf_t encode_buf;
    int encode_sparse_convert;
    int encode_sparse_ratio;
    int encode_sparse_safe;
    int encode_max_depth;
    int encode_invalid_numbers;
    int encode_number_precision;
    int encode_keep_buffer;
    int decode_invalid_numbers;
    int decode_max_depth;
} json_config_t;

typedef struct {
    const char *data;
    const char *ptr;
    strbuf_t *tmp;
    json_config_t *cfg;
    int current_depth;
} json_parse_t;

typedef struct {
    json_token_type_t type;
    int index;
    union {
        const char *string;
        double number;
        int boolean;
    } value;
    int string_len;
} json_token_t;

static const char *char2escape[256] = {
    "\\u0000", "\\u0001", "\\u0002", "\\u0003",
    "\\u0004", "\\u0005", "\\u0006", "\\u0007",
    "\\b", "\\t", "\\n", "\\u000b",
    "\\f", "\\r", "\\u000e", "\\u000f",
    "\\u0010", "\\u0011", "\\u0012", "\\u0013",
    "\\u0014", "\\u0015", "\\u0016", "\\u0017",
    "\\u0018", "\\u0019", "\\u001a", "\\u001b",
    "\\u001c", "\\u001d", "\\u001e", "\\u001f",
    NULL, NULL, "\\\"", NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, "\\/",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, "\\\\", NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, "\\u007f",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

static json_config_t *cfg;

static int json_destroy_config()
{
    if (cfg)
        strbuf_free(&cfg->encode_buf);
    cfg = NULL;
    return 0;
}

static void json_create_config()
{
    int i;
    cfg = (json_config_t*)malloc(sizeof(*cfg));

    cfg->encode_sparse_convert = DEFAULT_SPARSE_CONVERT;
    cfg->encode_sparse_ratio = DEFAULT_SPARSE_RATIO;
    cfg->encode_sparse_safe = DEFAULT_SPARSE_SAFE;
    cfg->encode_max_depth = DEFAULT_ENCODE_MAX_DEPTH;
    cfg->decode_max_depth = DEFAULT_DECODE_MAX_DEPTH;
    cfg->encode_invalid_numbers = DEFAULT_ENCODE_INVALID_NUMBERS;
    cfg->decode_invalid_numbers = DEFAULT_DECODE_INVALID_NUMBERS;
    cfg->encode_keep_buffer = DEFAULT_ENCODE_KEEP_BUFFER;
    cfg->encode_number_precision = DEFAULT_ENCODE_NUMBER_PRECISION;
#if DEFAULT_ENCODE_KEEP_BUFFER > 0
    strbuf_init(&cfg->encode_buf, 0);
#endif
    for (i = 0; i < 256; i++)
        cfg->ch2token[i] = T_ERROR;
    cfg->ch2token['{'] = T_OBJ_BEGIN;
    cfg->ch2token['}'] = T_OBJ_END;
    cfg->ch2token['['] = T_ARR_BEGIN;
    cfg->ch2token[']'] = T_ARR_END;
    cfg->ch2token[','] = T_COMMA;
    cfg->ch2token[':'] = T_COLON;
    cfg->ch2token['\0'] = T_END;
    cfg->ch2token[' '] = T_WHITESPACE;
    cfg->ch2token['\t'] = T_WHITESPACE;
    cfg->ch2token['\n'] = T_WHITESPACE;
    cfg->ch2token['\r'] = T_WHITESPACE;
    cfg->ch2token['f'] = T_UNKNOWN;
    cfg->ch2token['i'] = T_UNKNOWN;
    cfg->ch2token['I'] = T_UNKNOWN;
    cfg->ch2token['n'] = T_UNKNOWN;
    cfg->ch2token['N'] = T_UNKNOWN;
    cfg->ch2token['t'] = T_UNKNOWN;
    cfg->ch2token['"'] = T_UNKNOWN;
    cfg->ch2token['+'] = T_UNKNOWN;
    cfg->ch2token['-'] = T_UNKNOWN;
    for (i = 0; i < 10; i++)
        cfg->ch2token['0' + i] = T_UNKNOWN;
    for (i = 0; i < 256; i++)
        cfg->escape2char[i] = 0;
    cfg->escape2char['"'] = '"';
    cfg->escape2char['\\'] = '\\';
    cfg->escape2char['/'] = '/';
    cfg->escape2char['b'] = '\b';
    cfg->escape2char['t'] = '\t';
    cfg->escape2char['n'] = '\n';
    cfg->escape2char['f'] = '\f';
    cfg->escape2char['r'] = '\r';
    cfg->escape2char['u'] = 'u';
}

static void json_encode_exception(lua_State *l, json_config_t *cfg, strbuf_t *json, int lindex, const char *reason)
{
    if (!cfg->encode_keep_buffer)
        strbuf_free(json);
    luaL_error(l, "Cannot serialise %s: %s", lua_typename(l, lua_type(l, lindex)), reason);
}

static void json_append_string(lua_State *l, strbuf_t *json, int lindex)
{
    const char *escstr;
    int i;
    const char *str;
    size_t len;
    str = lua_tolstring(l, lindex, &len);
    strbuf_ensure_empty_length(json, len * 6 + 2);
    strbuf_append_char_unsafe(json, '\"');
    for (i = 0; i < len; i++)
    {
        escstr = char2escape[(unsigned char)str[i]];
        if (escstr)
            strbuf_append_string(json, escstr);
        else
            strbuf_append_char_unsafe(json, str[i]);
    }
    strbuf_append_char_unsafe(json, '\"');
}

static int lua_array_length(lua_State *l, json_config_t *cfg, strbuf_t *json)
{
    double k;
    int max;
    int items;
    max = 0;
    items = 0;
    lua_pushnil(l);
    while (lua_next(l, -2) != 0)
    {
        if (lua_type(l, -2) == LUA_TNUMBER && (k = lua_tonumber(l, -2)))
        {
            if (floor(k) == k && k >= 1)
            {
                if (k > max)
                    max = k;
                items++;
                lua_pop(l, 1);
                continue;
            }
        }
        lua_pop(l, 2);
        return -1;
    }
    if (cfg->encode_sparse_ratio > 0 &&
        max > items * cfg->encode_sparse_ratio &&
        max > cfg->encode_sparse_safe)
        {
            if (!cfg->encode_sparse_convert)
                json_encode_exception(l, cfg, json, -1, "excessively sparse array");
            return -1;
        }
    return max;
}

static void json_check_encode_depth(lua_State *l, json_config_t *cfg, int current_depth, strbuf_t *json)
{
    if (current_depth <= cfg->encode_max_depth && lua_checkstack(l, 3))
        return;
    if (!cfg->encode_keep_buffer)
        strbuf_free(json);
    luaL_error(l, "Cannot serialise, excessive nesting (%d)", current_depth);
}

static void json_append_data(lua_State *l, json_config_t *cfg, int current_depth, strbuf_t *json);
static void json_append_array(lua_State *l, json_config_t *cfg, int current_depth, strbuf_t *json, int array_length)
{
    int comma, i;
    strbuf_append_char(json, '[');
    comma = 0;
    for (i = 1; i <= array_length; i++)
    {
        if (comma)
            strbuf_append_char(json, ',');
        else
            comma = 1;
        lua_rawgeti(l, -1, i);
        json_append_data(l, cfg, current_depth, json);
        lua_pop(l, 1);
    }
    strbuf_append_char(json, ']');
}

static void json_append_number(lua_State *l, json_config_t *cfg, strbuf_t *json, int lindex)
{
    double num = lua_tonumber(l, lindex);
    int len;
    if (cfg->encode_invalid_numbers == 0)
    {
        if (isinf(num) || isnan(num))
            json_encode_exception(l, cfg, json, lindex, "must not be NaN or Inf");
    } else if (cfg->encode_invalid_numbers == 1)
    {
        if (isnan(num))
        {
            strbuf_append_mem(json, "nan", 3);
            return;
        }
    } else
    {
        if (isinf(num) || isnan(num))
        {
            strbuf_append_mem(json, "null", 4);
            return;
        }
    }
    strbuf_ensure_empty_length(json, FPCONV_G_FMT_BUFSIZE);
    len = fpconv_g_fmt(strbuf_empty_ptr(json), num, cfg->encode_number_precision);
    strbuf_extend_length(json, len);
}

static void json_append_object(lua_State *l, json_config_t *cfg, int current_depth, strbuf_t *json)
{
    int comma, keytype;
    strbuf_append_char(json, '{');
    lua_pushnil(l);
    comma = 0;
    while (lua_next(l, -2) != 0)
    {
        if (comma)
            strbuf_append_char(json, ',');
        else
            comma = 1;
        keytype = lua_type(l, -2);
        if (keytype == LUA_TNUMBER)
        {
            strbuf_append_char(json, '"');
            json_append_number(l, cfg, json, -2);
            strbuf_append_mem(json, "\":", 2);
        } else if (keytype == LUA_TSTRING)
        {
            json_append_string(l, json, -2);
            strbuf_append_char(json, ':');
        } else
        {
            json_encode_exception(l, cfg, json, -2, "table key must be a number or string");
        }
        json_append_data(l, cfg, current_depth, json);
        lua_pop(l, 1);
    }
    strbuf_append_char(json, '}');
}

static void json_append_data(lua_State *l, json_config_t *cfg, int current_depth, strbuf_t *json)
{
    int len;
    switch (lua_type(l, -1))
    {
        case LUA_TSTRING:
            json_append_string(l, json, -1);
            break;
        case LUA_TNUMBER:
            json_append_number(l, cfg, json, -1);
            break;
        case LUA_TBOOLEAN:
            if (lua_toboolean(l, -1))
                strbuf_append_mem(json, "true", 4);
            else
                strbuf_append_mem(json, "false", 5);
            break;
        case LUA_TTABLE:
            current_depth++;
            json_check_encode_depth(l, cfg, current_depth, json);
            len = lua_array_length(l, cfg, json);
            if (len > 0)
                json_append_array(l, cfg, current_depth, json, len);
            else
                json_append_object(l, cfg, current_depth, json);
            break;
        case LUA_TNIL:
            strbuf_append_mem(json, "null", 4);
            break;
        case LUA_TUSERDATA:
        case LUA_TFUNCTION:
        case LUA_TTHREAD:
            {
                unsigned long long pointer_addr = (unsigned long long)lua_topointer(l, -1);
                const int buff_size = 16;
                char str_buffer[buff_size];
                int len = snprintf(str_buffer, buff_size, "%llx", pointer_addr);
                strbuf_append_mem(json, "\"object at 0x", 13);
                strbuf_append_mem(json, str_buffer, len);
                strbuf_append_mem(json, "\"", 1);
                break;
            }
        case LUA_TLIGHTUSERDATA:
            if (lua_touserdata(l, -1) == NULL)
            {
                strbuf_append_mem(json, "null", 4);
                break;
            }
        default:
            /* Remaining types (LUA_TLIGHTUSERDATA) cannot be serialised */
            json_encode_exception(l, cfg, json, -1, "type not supported");
            /* never returns */
    }
}

#ifdef __cplusplus
extern "C" {
#endif

//
// Initialize CJSON
//
void YaMetrica_CJSON_Init()
{
    json_create_config();
}

//
// Finalize CJSON
//
void YaMetrica_CJSON_Final()
{
    json_destroy_config();
}

//
// Encode to JSON the top of the Lua stack.
//
char *YaMetrica_CJSON_Encode(lua_State *L, int arg, int* out_len, bool copy)
{
    lua_pushvalue(L, arg);
    strbuf_t *encode_buf;
    if (!cfg->encode_keep_buffer)
    {
        assert(0); // It's not supported mode.
    } else
    {
        encode_buf = &cfg->encode_buf;
        strbuf_reset(encode_buf);
    }
    json_append_data(L, cfg, 0, encode_buf);
    lua_pop(L, 1);

    char *str = strbuf_string(encode_buf, out_len);
    if (!copy) {
        return str;
    }
    
    char *str2 = (char *)malloc(*out_len);
    memcpy(str2, str, *out_len);
    return str2;
}

#ifdef __cplusplus
}
#endif

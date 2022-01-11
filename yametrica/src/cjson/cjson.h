#ifndef CJSON_H
#define CJSON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <dmsdk/sdk.h>

void YaMetrica_CJSON_Init();
void YaMetrica_CJSON_Final();
char *YaMetrica_CJSON_Encode(lua_State *l, int* out_len);

#ifdef __cplusplus
}
#endif

#endif
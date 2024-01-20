#include "leptjson.h"

#include <assert.h> /* assert() */
#include <math.h>
#include <stdint.h>
#include <stdlib.h> /* NULL, strtod() */

#define EXPECT(c, ch)         \
  do {                        \
    assert(*c->json == (ch)); \
    c->json++;                \
  } while (0)

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

typedef struct {
  const char* json;
} lept_context;

static void lept_parse_whitespace(lept_context* c) {
  const char* p = c->json;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
  c->json = p;
}

/// @brief 解析指定的字面量类型，如 null/true/false
/// @param c 上下文
/// @param v json 值
/// @param seq 字面量
/// @param type 字面量对应类型
/// @return 状态码
static int lept_parse_literal(lept_context* c, lept_value* v, const char* seq, lept_type type) {
  assert(*seq != '\0');
  EXPECT(c, *seq);
  for (const char* p = seq + 1; *p; p++, c->json++) {
    if (*p != *c->json) return LEPT_PARSE_INVALID_VALUE;
  }
  v->type = type;
  return LEPT_PARSE_OK;
}

// 解析数值类型
static int lept_parse_number(lept_context* c, lept_value* v) {
  const char* p = c->json;
  // 对接下来内容进行校验，判断是否存在满足 json
  // 数值定义的前缀，并尽可能寻找最长的前缀（如果存在数值前缀，指针p指向最长前缀的下一个位置）

  // 1.可选的负号
  if (*p == '-') p++;
  // 2.必选整数部分（要么为 0，要么为任意正数）
  if (*p == '0') {
    p++;
  } else {
    if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
    p++;
    while (ISDIGIT(*p)) p++;
  }
  // 3.可选小数部分 .\d+
  if (*p == '.') {
    p++;
    // 因为小数部分是可选的，正常来说如果寻找前缀的过程中如果发现小数部分非法了，是需要终止寻找前缀直接进入前缀的解析过程的，
    // 这里的一个 trick 是一旦发现小数部分非法，说明后续即使成功解析已有前缀，也会因为 JSON 值后面存在非空白内容导致
    // JSON 值非法，可提前返回错误（下面的指数部分同理）
    if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
    p++;
    while (ISDIGIT(*p)) p++;
  }
  // 4.可选指数部分 [eE][+-]\d+
  if (*p == 'e' || *p == 'E') {
    p++;
    if (*p == '+' || *p == '-') p++;
    // PS：指数部分是可以允许前导 0 的
    if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
    p++;
    while (ISDIGIT(*p)) p++;
  }

  // strtod 解析规则 https://www.apiref.com/cpp-zh/c/string/byte/strtof.html
  double parse_res = strtod(c->json, NULL);
  if (parse_res == HUGE_VAL || parse_res == -HUGE_VAL) return LEPT_PARSE_NUMBER_TOO_BIG;
  c->json = p;
  v->n = parse_res;
  v->type = LEPT_NUMBER;
  return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
  switch (*c->json) {
    case 't':
      return lept_parse_literal(c, v, "true", LEPT_TRUE);
    case 'f':
      return lept_parse_literal(c, v, "false", LEPT_FALSE);
    case 'n':
      return lept_parse_literal(c, v, "null", LEPT_NULL);
    default:
      return lept_parse_number(c, v);
    case '\0':
      return LEPT_PARSE_EXPECT_VALUE;
  }
}

int lept_parse(lept_value* v, const char* json) {
  lept_context c;
  assert(v != NULL);
  c.json = json;
  v->type = LEPT_NULL;
  lept_parse_whitespace(&c);
  // 即使目标前缀解析成功，也要检查其之后是否存在非空白内容，如果存在，则本次解析失败
  uint8_t res = lept_parse_value(&c, v);
  if (res == LEPT_PARSE_OK) {
    lept_parse_whitespace(&c);
    return c.json[0] == '\0' ? res : LEPT_PARSE_ROOT_NOT_SINGULAR;
  }
  return res;
}

lept_type lept_get_type(const lept_value* v) {
  assert(v != NULL);
  return v->type;
}

double lept_get_number(const lept_value* v) {
  assert(v != NULL && v->type == LEPT_NUMBER);
  return v->n;
}

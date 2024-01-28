#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <assert.h> /* assert() */
#include <ctype.h>
#include <errno.h> /* errno, ERANGE */
#include <math.h>  /* HUGE_VAL */
#include <stdbool.h>
#include <stdlib.h> /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h> /* memcpy() */

#include "leptjson.h"

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)         \
  do {                        \
    assert(*c->json == (ch)); \
    c->json++;                \
  } while (0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)                                    \
  do {                                                 \
    *(char*)lept_context_push(c, sizeof(char)) = (ch); \
  } while (0)

typedef struct {
  const char* json;
  char* stack;
  size_t size, top;
} lept_context;

static void* lept_context_push(lept_context* c, size_t size) {
  void* ret;
  assert(size > 0);
  if (c->top + size >= c->size) {
    if (c->size == 0) c->size = LEPT_PARSE_STACK_INIT_SIZE;
    while (c->top + size >= c->size) c->size += c->size >> 1; /* c->size * 1.5 */
    c->stack = (char*)realloc(c->stack, c->size);
  }
  ret = c->stack + c->top;
  c->top += size;
  return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
  assert(c->top >= size);
  return c->stack + (c->top -= size);
}

static void lept_parse_whitespace(lept_context* c) {
  const char* p = c->json;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
  c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
  size_t i;
  EXPECT(c, literal[0]);
  for (i = 0; literal[i + 1]; i++)
    if (c->json[i] != literal[i + 1]) return LEPT_PARSE_INVALID_VALUE;
  c->json += i;
  v->type = type;
  return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
  const char* p = c->json;
  if (*p == '-') p++;
  if (*p == '0')
    p++;
  else {
    if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
    for (p++; ISDIGIT(*p); p++)
      ;
  }
  if (*p == '.') {
    p++;
    if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
    for (p++; ISDIGIT(*p); p++)
      ;
  }
  if (*p == 'e' || *p == 'E') {
    p++;
    if (*p == '+' || *p == '-') p++;
    if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
    for (p++; ISDIGIT(*p); p++)
      ;
  }
  errno = 0;
  v->u.n = strtod(c->json, NULL);
  if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL)) return LEPT_PARSE_NUMBER_TOO_BIG;
  v->type = LEPT_NUMBER;
  c->json = p;
  return LEPT_PARSE_OK;
}

// 判断给定字符是否为合法十六进制，如果是将对应值写入 num
static bool is_hex_char(char ch, unsigned* num) {
  if (ch >= '0' && ch <= '9') {
    *num = ch - '0';
    return true;
  }
  if ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) {
    *num = tolower(ch) - 'a' + 10;
    return true;
  }
  return false;
}

// 解析4位十六进制数为无符号十进制数，如果解析成功将结果写入 u 并返回指向下一个解析位置的指针；否则返回空指针
static const char* lept_parse_hex4(const char* p, unsigned* u) {
  unsigned num, res = 0;
  for (int exp = 3; exp >= 0; exp--, p++) {
    if (is_hex_char(*p, &num)) {
      res += num * pow(16, exp);
    } else {
      return NULL;
    }
  }
  *u = res;
  return p;
}

///| 码点范围            | 码点位数  | 字节1     | 字节2    | 字节3    | 字节4     |
///|:------------------:|:--------:|:--------:|:--------:|:--------:|:--------:|
///| U+0000 ~ U+007F    | 7        | 0xxxxxxx |
///| U+0080 ~ U+07FF    | 11       | 110xxxxx | 10xxxxxx |
///| U+0800 ~ U+FFFF    | 16       | 1110xxxx | 10xxxxxx | 10xxxxxx |
///| U+10000 ~ U+10FFFF | 21       | 11110xxx | 10xxxxxx | 10xxxxxx | 10xxxxxx |

/// @brief 将码点以 utf8 编码写入缓冲区
/// @param c 上下文
/// @param u unicode 码点
static void lept_encode_utf8(lept_context* c, unsigned u) {
  if (u <= 0x7F) { // 码点位数 <= 7
    PUTC(c, u);
  } else if (u < 0x7FF) { // 码点位数 8-11
    // utf8 码元分别为：码点的 6+ 位、码点的低 6 位
    PUTC(c, 0b11000000 | u >> 6);
    PUTC(c, 0b10000000 | u & 0b00111111);
  } else if (u < 0xFFFF) { // 码点位数 12-16
    // utf8 码元分别为：码点的 12+ 位、码点的 7-12 位、码点的低 6 位
    PUTC(c, 0b11100000 | u >> 12);
    PUTC(c, 0b10000000 | u >> 6 & 0b00111111);
    PUTC(c, 0b10000000 | u & 0b00111111);
  } else { // 码点位数 17-21
    // utf8 码元分别为：码点的 18+ 位、码点的 13-18 位、码点的 7-12 位、码点的低 6 位
    PUTC(c, 0b11110000 | u >> 18);
    PUTC(c, 0b10000000 | u >> 12 & 0b00111111);
    PUTC(c, 0b10000000 | u >> 6 & 0b00111111);
    PUTC(c, 0b10000000 | u & 0b00111111);
  }
}

#define STRING_ERROR(ret) \
  do {                    \
    c->top = head;        \
    return ret;           \
  } while (0)

static int lept_parse_string(lept_context* c, lept_value* v) {
  size_t head = c->top, len;
  // unicode 码点
  unsigned u;
  const char* p;
  EXPECT(c, '\"');
  p = c->json;
  for (;;) {
    char ch = *p++;
    switch (ch) {
      case '\"':
        len = c->top - head;
        lept_set_string(v, (const char*)lept_context_pop(c, len), len);
        c->json = p;
        return LEPT_PARSE_OK;
      case '\\':
        switch (*p++) {
          case '\"':
            PUTC(c, '\"');
            break;
          case '\\':
            PUTC(c, '\\');
            break;
          case '/':
            PUTC(c, '/');
            break;
          case 'b':
            PUTC(c, '\b');
            break;
          case 'f':
            PUTC(c, '\f');
            break;
          case 'n':
            PUTC(c, '\n');
            break;
          case 'r':
            PUTC(c, '\r');
            break;
          case 't':
            PUTC(c, '\t');
            break;
          case 'u':
            // clang-format off
            // JSON 代理对详解：https://huangtengxiao.gitee.io/post/Unicode%E4%BB%A3%E7%90%86%E5%AF%B9.html
            // JSON 使用 \u 转义来表示 unicode 字符，并且这些 unicode 字符采用 utf-16 编码。
            // utf-16 属于变长编码，其码元大小为 16 位，unicode 字符需要使用需要 1 个或 2 个码元表示：
            // 1.当 unicode 字符的码点落在 BMP（基本多文种平面，范围 0000-FFFF）时，此时单个码元即可表示该码点，无需任何转换。
            // 2.当 unicode 字符的码点超出 BMP 范围（超出 16 位），就需要使用两个码元（也叫代理对）进行表示，
            // 其中高位码元的范围落在 D800-DBFF，低位码元的范围落在 DC00-DFFF（D800-DFFF 是 BMP 中预留给代理对的区域，这样在解析的过程中发现位于代理区的码元时，就知道接下来需要后续两个码元表示一个 BMP 范围外的字符），
            // 高位码元作为码点的高 10 位，低位码元作为码点的低 10 位，可表示高达 20 位的空间，又因代理对负只表示 BMP 范围外的码元，实际上需要将这 20 位编码空间映射到 010000-10FFFF 对应的码点，
            // 映射的公式为：codepoint = 0x10000 + (H − 0xD800) × 0x400 + (L − 0xDC00)
            // clang-format on
            if (!(p = lept_parse_hex4(p, &u))) STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
            // 处理代理对
            if (u >= 0xD800 && u <= 0xDFFF) {
              // 第一个必须为高代理项，后接一个低代理项
              if (u >= 0xDC00) STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
              if (*p++ == '\\' && *p++ == 'u') {
                unsigned l;
                if (!(p = lept_parse_hex4(p, &l))) STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                if (l < 0xDC00 || l > 0xDFFF) STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                u = 0x10000 + ((u - 0xD800) << 10) + (l - 0xDC00);
              } else {
                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
              }
            }
            lept_encode_utf8(c, u);
            break;
          default:
            STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);
        }
        break;
      case '\0':
        STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK);
      default:
        if ((unsigned char)ch < 0x20) STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);
        PUTC(c, ch);
    }
  }
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
    case '"':
      return lept_parse_string(c, v);
    case '\0':
      return LEPT_PARSE_EXPECT_VALUE;
  }
}

int lept_parse(lept_value* v, const char* json) {
  lept_context c;
  int ret;
  assert(v != NULL);
  c.json = json;
  c.stack = NULL;
  c.size = c.top = 0;
  lept_init(v);
  lept_parse_whitespace(&c);
  if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
    lept_parse_whitespace(&c);
    if (*c.json != '\0') {
      v->type = LEPT_NULL;
      ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
    }
  }
  assert(c.top == 0);
  free(c.stack);
  return ret;
}

void lept_free(lept_value* v) {
  assert(v != NULL);
  if (v->type == LEPT_STRING) free(v->u.s.s);
  v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v) {
  assert(v != NULL);
  return v->type;
}

int lept_get_boolean(const lept_value* v) {
  assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
  return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b) {
  lept_free(v);
  v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

double lept_get_number(const lept_value* v) {
  assert(v != NULL && v->type == LEPT_NUMBER);
  return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
  lept_free(v);
  v->u.n = n;
  v->type = LEPT_NUMBER;
}

const char* lept_get_string(const lept_value* v) {
  assert(v != NULL && v->type == LEPT_STRING);
  return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v) {
  assert(v != NULL && v->type == LEPT_STRING);
  return v->u.s.len;
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
  assert(v != NULL && (s != NULL || len == 0));
  lept_free(v);
  v->u.s.s = (char*)malloc(len + 1);
  memcpy(v->u.s.s, s, len);
  v->u.s.s[len] = '\0';
  v->u.s.len = len;
  v->type = LEPT_STRING;
}

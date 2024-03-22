#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <stddef.h> /* size_t */

typedef enum {
  LEPT_NULL,
  LEPT_FALSE,
  LEPT_TRUE,
  LEPT_NUMBER,
  LEPT_STRING,
  LEPT_ARRAY,
  LEPT_OBJECT
} lept_type;

typedef struct lept_value lept_value;
typedef struct lept_member lept_member;

struct lept_value {
  union {
    struct {
      lept_member* m; // 键值对集合
      size_t size;    // 键值对个数
    } o;              // 对象
    struct {
      lept_value* e; // 数组起始地址
      size_t size;   // 数组元素个数
    } a;             // 数组
    struct {
      char* s;
      size_t len;
    } s;      // 字符串
    double n; // 数值
  } u;
  lept_type type;
};

// 键值对
struct lept_member {
  char* k;      // 键
  size_t klen;  // 键长度
  lept_value v; // 值
};

enum {
  LEPT_PARSE_OK,                    // 解析成功
  LEPT_PARSE_EXPECT_VALUE,          // 解析目标为空
  LEPT_PARSE_INVALID_VALUE,         // 解析目标类型非法
  LEPT_PARSE_ROOT_NOT_SINGULAR,     // 解析目标成功之后，空白之后还有值，如 {"a": 114514 1919}
  LEPT_PARSE_NUMBER_TOO_BIG,        // 数值解析溢出
  LEPT_PARSE_MISS_QUOTATION_MARK,   // 字符串缺失引号
  LEPT_PARSE_INVALID_STRING_ESCAPE, // 字符串非法转义
  LEPT_PARSE_INVALID_STRING_CHAR,   // 非法字符，比如码点为 0x0 ~ 0x1f 的不可见字符
  LEPT_PARSE_INVALID_UNICODE_HEX,   // 代理项后没有后接4位十六进制数
  LEPT_PARSE_INVALID_UNICODE_SURROGATE, // 非法代理对（只存在高代理缺失低代理/低代理码点范围非法）
  LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, // 数组缺失逗号或者中括号
  LEPT_PARSE_MISS_KEY,                     // 对象缺失键
  LEPT_PARSE_MISS_COLON,                   // 对象缺失冒号
  LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET   // 对象缺失逗号或者大括号
};

#define lept_init(v)       \
  do {                     \
    (v)->type = LEPT_NULL; \
  } while (0)

int lept_parse(lept_value* v, const char* json);

void lept_free(lept_value* v);
void lept_free_member(lept_member* m);

lept_type lept_get_type(const lept_value* v);

#define lept_set_null(v) lept_free(v)

int lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value* v, int b);

double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double n);

const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);

size_t lept_get_array_size(const lept_value* v);
lept_value* lept_get_array_element(const lept_value* v, size_t index);

size_t lept_get_object_size(const lept_value* v);
const char* lept_get_object_key(const lept_value* v, size_t index);
size_t lept_get_object_key_length(const lept_value* v, size_t index);
lept_value* lept_get_object_value(const lept_value* v, size_t index);

#endif /* LEPTJSON_H__ */

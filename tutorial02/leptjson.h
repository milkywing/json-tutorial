#ifndef LEPTJSON_H__
#define LEPTJSON_H__

typedef enum {
  LEPT_NULL,
  LEPT_FALSE,
  LEPT_TRUE,
  LEPT_NUMBER,
  LEPT_STRING,
  LEPT_ARRAY,
  LEPT_OBJECT
} lept_type;

typedef struct {
  // 数值
  double n;
  lept_type type;
} lept_value;

enum {
  LEPT_PARSE_OK,                // 解析成功
  LEPT_PARSE_EXPECT_VALUE,      // 解析目标为空
  LEPT_PARSE_INVALID_VALUE,     // 解析目标类型非法
  LEPT_PARSE_ROOT_NOT_SINGULAR, // 解析目标成功之后，空白之后还有值，如 {"a": 114514 1919}
  LEPT_PARSE_NUMBER_TOO_BIG     // 数值解析溢出
};

int lept_parse(lept_value* v, const char* json);

lept_type lept_get_type(const lept_value* v);

double lept_get_number(const lept_value* v);

#endif /* LEPTJSON_H__ */

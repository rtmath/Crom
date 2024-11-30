#include <errno.h>
#include <stdio.h>  // for printf
#include <stdlib.h> // for malloc
#include <string.h> // for strcmp

#include "common.h"
#include "error.h"
#include "value.h"

#define BASE_DECIMAL 10
#define BASE_HEX 16
#define BASE_BINARY 2

static char *ExtractString(Token token) {
  char *str = malloc(sizeof(char) * (token.length + ROOM_FOR_NULL_BYTE));
  for (int i = 0; i < token.length; i++) {
    str[i] = token.position_in_source[i];
  }
  str[token.length] = '\0';

  return str;
}

Value NewValue(Type type, Token token) {
  const int base =
    (token.type == HEX_LITERAL)
    ? BASE_HEX
    : (token.type == BINARY_LITERAL)
      ? BASE_BINARY
      : BASE_DECIMAL;

  Value ret_val = { 0 };

  if (TypeIs_None(type)) {
    return ret_val;

  } else if (TypeIs_Int(type)) {
    if (Int64Overflow(token, base)) {
      ERROR_AT_TOKEN(token, "I64 Overflow\n", "");
      return (Value){ .type = NoType(), .as.integer = 0 };
    }

    int64_t integer = TokenToInt64(token, base);
    return NewIntValue(integer);

  } else if (TypeIs_Uint(type)) {
    if (Uint64Overflow(token, base)) {
      ERROR_AT_TOKEN(token, "U64 Overflow\n", "");
      return (Value){ .type = NoType(), .as.uinteger = 0 };
    }

    uint64_t unsignedint = TokenToUint64(token, base);
    return NewUintValue(unsignedint);

  } else if (TypeIs_Float(type)) {
    if (DoubleOverflow(token) || DoubleUnderflow(token)) {
      ERROR_AT_TOKEN(token, "F64 Over/Underflow\n", "");
      return (Value){ .type = NoType(), .as.floating = 0 };
    }

    double d = TokenToDouble(token);
    return NewFloatValue(d);

  } else if (TypeIs_Bool(type)) {
    char *s = ExtractString(token);
    Value b_return = NewBoolValue((strcmp(s, "true") == 0) ? true : false);

    free(s);
    return b_return;

  } else if (TypeIs_Char(type)) {
    char *s = ExtractString(token);
    Value c_return = NewCharValue(s[0]);
    free(s);

    return c_return;

  } else if (TypeIs_String(type)) {
    char *s = ExtractString(token);
    return NewStringValue(s);

  } else {
    ERROR_AND_EXIT_FMTMSG("NewValue(): '%s' not implemented yet", TypeTranslation(type));
  }

  return ret_val;
}

Value NewIntValue(int64_t i) {
  return (Value){
    .type = SmallestContainingIntType(i),
    .as.integer = i,
  };
}

Value NewUintValue(uint64_t u) {
  return (Value){
    .type = SmallestContainingUintType(u),
    .as.uinteger = u,
  };
}

Value NewFloatValue(double d) {
  Value v = (Value){
    .type = SmallestContainingFloatType(d),
    .as.floating = d,
  };

  return v;
}

Value NewCharValue(char c) {
  return (Value){
    .type = NewType(CHAR),
    .as.character = c,
  };
}

Value NewStringValue(const char *s) {
  return (Value){
    .type = NewArrayType(STRING, strlen(s)),
    .as.string = s,
  };
}

Value NewBoolValue(bool b)  {
  return (Value){
    .type = NewType(BOOL),
    .as.boolean = b,
  };
}

Value AddValues(Value v1, Value v2) {
  if (TypeIs_Int(v1.type)) return NewIntValue(v1.as.integer + v2.as.integer);
  if (TypeIs_Uint(v1.type)) return NewUintValue(v1.as.uinteger + v2.as.uinteger);
  if (TypeIs_Float(v1.type)) return NewFloatValue(v1.as.floating + v2.as.floating);

  return (Value){0};
}

Value SubValues(Value v1, Value v2) {
  if (TypeIs_Int(v1.type)) return NewIntValue(v1.as.integer - v2.as.integer);
  if (TypeIs_Uint(v1.type)) return NewUintValue(v1.as.uinteger - v2.as.uinteger);
  if (TypeIs_Float(v1.type)) return NewFloatValue(v1.as.floating - v2.as.floating);

  return (Value){0};
}

Value MulValues(Value v1, Value v2) {
  if (TypeIs_Int(v1.type)) return NewIntValue(v1.as.integer * v2.as.integer);
  if (TypeIs_Uint(v1.type)) return NewUintValue(v1.as.uinteger * v2.as.uinteger);
  if (TypeIs_Float(v1.type)) return NewFloatValue(v1.as.floating * v2.as.floating);

  return (Value){0};
}

Value DivValues(Value v1, Value v2) {
  if (TypeIs_Int(v1.type)) return NewIntValue(v1.as.integer / v2.as.integer);
  if (TypeIs_Uint(v1.type)) return NewUintValue(v1.as.uinteger / v2.as.uinteger);
  if (TypeIs_Float(v1.type)) return NewFloatValue(v1.as.floating / v2.as.floating);

  return (Value){0};
}

Value ModValues(Value v1, Value v2) {
  if (TypeIs_Int(v1.type)) return NewIntValue(v1.as.integer % v2.as.integer);
  if (TypeIs_Uint(v1.type)) return NewUintValue(v1.as.uinteger % v2.as.uinteger);

  return (Value){0};
}

Value Not(Value v) {
  return NewBoolValue(!v.as.boolean);
}

Value Equality(Value v1, Value v2) {
  if (TypeIs_Int(v1.type)) return NewBoolValue(v1.as.integer == v2.as.integer);
  if (TypeIs_Uint(v1.type)) return NewBoolValue(v1.as.uinteger == v2.as.uinteger);
  if (TypeIs_Float(v1.type)) return NewBoolValue(v1.as.floating == v2.as.floating);
  if (TypeIs_Char(v1.type)) return NewBoolValue(v1.as.character == v2.as.character);
  if (TypeIs_Bool(v1.type)) return NewBoolValue(v1.as.boolean == v2.as.boolean);

  return (Value){0};
}

Value GreaterThan(Value v1, Value v2) {
  if (TypeIs_Int(v1.type)) return NewBoolValue(v1.as.integer > v2.as.integer);
  if (TypeIs_Uint(v1.type)) return NewBoolValue(v1.as.uinteger > v2.as.uinteger);
  if (TypeIs_Float(v1.type)) return NewBoolValue(v1.as.floating > v2.as.floating);

  return (Value){0};
}

Value LessThan(Value v1, Value v2) {
  if (TypeIs_Int(v1.type)) return NewBoolValue(v1.as.integer < v2.as.integer);
  if (TypeIs_Uint(v1.type)) return NewBoolValue(v1.as.uinteger < v2.as.uinteger);
  if (TypeIs_Float(v1.type)) return NewBoolValue(v1.as.floating < v2.as.floating);

  return (Value){0};
}

Value LogicalAND(Value v1, Value v2) {
  if (!TypeIs_Bool(v1.type) || !TypeIs_Bool(v2.type)) ERROR_AND_EXIT("LogicalAND(): Cannot compare non-bool types");
  if (!TypesMatchExactly(v1.type, v2.type)) ERROR_AND_EXIT("LogicalAND(): Type mismatch");

  return NewBoolValue(v1.as.boolean && v2.as.boolean);
}

Value LogicalOR(Value v1, Value v2) {
  if (!TypeIs_Bool(v1.type) || !TypeIs_Bool(v2.type)) ERROR_AND_EXIT("LogicalAND(): Cannot compare non-bool types");
  if (!TypesMatchExactly(v1.type, v2.type)) ERROR_AND_EXIT("LogicalAND(): Type mismatch");

  return NewBoolValue(v1.as.boolean || v2.as.boolean);
}

void InlinePrintValue(Value v) {
  if (TypeIs_None(v.type)) {
    printf("NONE");
    return;
  }

  if (TypeIs_Int(v.type)) {
    InlinePrintType(v.type);
    printf(": %ld", v.as.integer);
    return;
  }

  if (TypeIs_Uint(v.type)) {
    InlinePrintType(v.type);
    printf(": %lu", v.as.uinteger);
    return;
  }

  if (TypeIs_Float(v.type)) {
    InlinePrintType(v.type);
    printf(": %f", v.as.floating);
    return;
  }

  if (TypeIs_Char(v.type)) {
    InlinePrintType(v.type);
    printf(": %c", v.as.character);
    return;
  }

  if (TypeIs_String(v.type)) {
    InlinePrintType(v.type);
    printf(": %s", v.as.string);
    return;
  }

  if (TypeIs_Bool(v.type)) {
    InlinePrintType(v.type);
    printf(": %s", (v.as.boolean) ? "true" : "false");
    return;
  }
}

void PrintValue(Value v) {
  InlinePrintValue(v);
  printf("\n");
}

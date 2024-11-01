#ifndef AST_H
#define AST_H

#include <stdbool.h>

#include "parser_annotation.h"
#include "symbol_table.h"
#include "token.h"

#define LEFT 0
#define RIGHT 1
#define MIDDLE 2

typedef enum {
  NO_ARITY = 0,
  UNARY_ARITY = 1,
  BINARY_ARITY = 2,
  TERNARY_ARITY = 3,
} Arity;

typedef enum {
  UNTYPED,
  START_NODE,
  CHAIN_NODE,
  STATEMENT_NODE,
  DECLARATION_NODE,
  IDENTIFIER_NODE,
  ENUM_IDENTIFIER_NODE,
  ARRAY_SUBSCRIPT_NODE,

  IF_NODE,
  WHILE_NODE,

  BREAK_NODE,
  CONTINUE_NODE,
  RETURN_NODE,

  FUNCTION_NODE,
  FUNCTION_RETURN_TYPE_NODE,
  FUNCTION_PARAM_NODE,
  FUNCTION_BODY_NODE,

  LITERAL_NODE,

  ASSIGNMENT_NODE,
  UNARY_OP_NODE,
  BINARY_OP_NODE,

  PREFIX_INCREMENT_NODE,
  PREFIX_DECREMENT_NODE,
  POSTFIX_INCREMENT_NODE,
  POSTFIX_DECREMENT_NODE,

  NODE_TYPE_COUNT
} NodeType;

typedef struct AST_Node {
  Token token;
  Arity arity;
  NodeType type;
  ParserAnnotation annotation;

  struct AST_Node *nodes[3];
} AST_Node;

AST_Node *NewNode(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, ParserAnnotation annotation);
AST_Node *NewNodeFromToken(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Token token, ParserAnnotation annotation);
AST_Node *NewNodeFromSymbol(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Symbol symbol);
AST_Node *NewNodeWithArity(NodeType type, AST_Node *left, AST_Node *middle, AST_Node *right, Arity arity, ParserAnnotation annotation);

const char *NodeTypeTranslation(NodeType t);

void PrintAST(AST_Node *root);
void PrintNode(AST_Node *node);

#endif

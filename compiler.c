#include <stdarg.h> // for va_list
#include <stdbool.h>
#include <stdio.h>  // for printf and friends

#include "ast.h"
#include "common.h"
#include "compiler.h"
#include "error.h"
#include "lexer.h"

typedef AST_Node* (*ParseFn)();
AST_Node *ParseTree;

struct {
  Token current;
  Token next;
} Parser;

typedef enum {
  PREC_EOF = -1,
  NO_PRECEDENCE,
  TERM,
  FACTOR,
  UNARY,
} Precedence;

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

static AST_Node *Type();
static AST_Node *Identifier();
static AST_Node *Number();
static AST_Node *Unary();
static AST_Node *Binary();
static AST_Node *Parens();
static AST_Node *Expression();
static AST_Node *Statement();

ParseRule Rules[] = {
  // Type Keywords
  [I8]             = {   Type,   NULL, NO_PRECEDENCE },
  [I16]            = {   Type,   NULL, NO_PRECEDENCE },
  [I32]            = {   Type,   NULL, NO_PRECEDENCE },
  [I64]            = {   Type,   NULL, NO_PRECEDENCE },

  [U8]             = {   Type,   NULL, NO_PRECEDENCE },
  [U16]            = {   Type,   NULL, NO_PRECEDENCE },
  [U32]            = {   Type,   NULL, NO_PRECEDENCE },
  [U64]            = {   Type,   NULL, NO_PRECEDENCE },

  [F32]            = {   Type,   NULL, NO_PRECEDENCE },
  [F64]            = {   Type,   NULL, NO_PRECEDENCE },

  [CHAR]           = {   Type,   NULL, NO_PRECEDENCE },
  [STRING]         = {   Type,   NULL, NO_PRECEDENCE },

  [BOOL]           = {   Type,   NULL, NO_PRECEDENCE },
  [VOID]           = {   Type,   NULL, NO_PRECEDENCE },
  [ENUM]           = {   Type,   NULL, NO_PRECEDENCE },
  [STRUCT]         = {   Type,   NULL, NO_PRECEDENCE },

  [IDENTIFIER]     = { Identifier, NULL, NO_PRECEDENCE },

  // Constants
  [INT_CONSTANT]   = { Number,   NULL, NO_PRECEDENCE },
  [FLOAT_CONSTANT] = { Number,   NULL, NO_PRECEDENCE },

  [STRING_LITERAL] = {   NULL,   NULL, NO_PRECEDENCE },

  // Punctuators
  [LPAREN]         = { Parens,   NULL, NO_PRECEDENCE },
  [PLUS]           = {   NULL, Binary,          TERM },
  [MINUS]          = {  Unary, Binary,          TERM },
  [ASTERISK]       = {   NULL, Binary,        FACTOR },
  [DIVIDE]         = {   NULL, Binary,        FACTOR },

  // Misc
  [TOKEN_EOF]      = {   NULL,   NULL,      PREC_EOF },
};

void Advance() {
  Parser.current = Parser.next;
  Parser.next = ScanToken();

  if (Parser.next.type != ERROR) return;

  ERROR_AND_EXIT("Advance(): Error token encountered after token '%s'",
      TokenTypeTranslation(Parser.next.type));
}

bool NextTokenIs(TokenType type) {
  return (Parser.next.type == type);
}

bool Match(TokenType type) {
  if (!NextTokenIs(type)) return false;

  Advance();

  return true;
}

void Consume(TokenType type, const char *msg, ...) {
  if (NextTokenIs(type)) {
    Advance();
    return;
  }

  va_list args;
  va_start(args, msg);

  ERROR_AND_EXIT_VALIST(msg, args);

  va_end(args);
}

AST_Node *Parse(int PrecedenceLevel) {
  if (PrecedenceLevel == PREC_EOF) return NULL;
  Advance();

  AST_Node *return_node = NULL;

  ParseFn prefix_rule = Rules[Parser.current.type].prefix;
  if (prefix_rule == NULL) {
    printf("Prefix Rule for '%s' is NULL.\n",
        TokenTypeTranslation(Parser.current.type));
    return NULL;
  }

  AST_Node *prefix_node = prefix_rule();

  while (PrecedenceLevel <= Rules[Parser.next.type].precedence) {
    Advance();

    ParseFn infix_rule = Rules[Parser.current.type].infix;
    if (infix_rule == NULL) {
      ERROR_AND_EXIT("Infix Rule for '%s' is NULL.\n",
                     TokenTypeTranslation(Parser.current.type));
    }

    AST_Node *infix_node = infix_rule();

    if (return_node == NULL) {
      SetLeftChild(infix_node, prefix_node);
    } else {
      SetLeftChild(infix_node, return_node);
      return_node = infix_node;
    }

    return_node = infix_node;
  }

  return (return_node == NULL) ? prefix_node : return_node;
}

static AST_Node *Number() {
  AST_Unary_Node *n = NewUnaryNode();

  n->node.token = Parser.current;

  return (AST_Node*)n;
}

static AST_Node *Type() {
  Token remember_token = Parser.current;
  AST_Unary_Node *n = NewUnaryNode();

  n->node.token = remember_token;
  Consume(IDENTIFIER, "Expected IDENTIFIER after Type '%s', got '%s' instead.",
          TokenTypeTranslation(remember_token.type),
          TokenTypeTranslation(Parser.next.type));

  SetLeftChild(AS_NODE(n), Identifier());

  return (AST_Node*)n;
}

static AST_Node *Identifier() {
  AST_Unary_Node *n = NewUnaryNode();
  n->node.token = Parser.current;

  if (Match(EQUALS)) {
    SetLeftChild(AS_NODE(n), Expression());
  } else if (NextTokenIs(SEMICOLON)) {
    // TODO: Variable declaration
  } else {
    ERROR_AND_EXIT("Expected '=' or ';' after identifier '%.*s', got '%s' instead",
        Parser.current.length,
        Parser.current.position_in_source,
        TokenTypeTranslation(Parser.next.type));
  }

  return (AST_Node*)n;
}

static AST_Node *Unary() {
  Token remember_token = Parser.current;

  AST_Unary_Node *n = NewUnaryNode();
  AST_Node *parse_result = Parse(UNARY);
  SetLeftChild(AS_NODE(n), parse_result);

  switch(remember_token.type) {
    case MINUS:
      n->node.token = remember_token;
      return (AST_Node*)n;
    default:
      printf("Unknown Unary operator '%s'\n",
          TokenTypeTranslation(remember_token.type));
      return (AST_Node*)n;
  }
}

static AST_Node *Binary() {
  Precedence precedence = Rules[Parser.current.type].precedence;
  Token remember_token = Parser.current;
  AST_Binary_Node *n = NewBinaryNode();

  AST_Node *parse_result = Parse(precedence + 1);
  SetRightChild(AS_NODE(n), parse_result);

  switch(remember_token.type) {
    case PLUS:
    case MINUS:
    case ASTERISK:
    case DIVIDE:
      n->node.token = remember_token;
      return (AST_Node*)n;
    default:
      printf("Binary(): Unknown operator '%s'\n", TokenTypeTranslation(remember_token.type));
      return (AST_Node*)n;
  }
}

static AST_Node *Expression() {
  return Parse((Precedence)1);
}

static AST_Node *Statement() {
  AST_Node *expr_result = Expression();
  Consume(SEMICOLON, "A ';' is expected after an expression statement, got '%s' instead",
      TokenTypeTranslation(Parser.next.type));

  return expr_result;
}

static AST_Node *Parens() {
  AST_Node *n = Expression();
  Consume(RPAREN, "Missing ')' after expression");

  return n;
}

static void PrintASTRecurse(AST_Node *node, int depth, char label) {
  if (node == NULL) return;

  char buf[100] = {0};
  int i = 0;
  for (; i < depth * 4 && i + node->token.length < 100; i++) {
    buf[i] = ' ';
  }
  buf[i] = '\0';
  printf("%s%c: %.*s\n", buf, label,
      node->token.length,
      node->token.position_in_source);

  PrintASTRecurse(AS_UNARY(node)->left, depth + 1, 'L');
  if (node->arity == AST_TERNARY) PrintASTRecurse(AS_TERNARY(node)->middle, depth + 1, 'M');
  if (node->arity == AST_BINARY) PrintASTRecurse(AS_BINARY(node)->right, depth + 1, 'R');
}

static void PrintAST(AST_Node *root) {
  PrintASTRecurse(root, 0, 'S');
}

void Compile(const char *source) {
  InitLexer(source);

  Advance();

  while (!Match(TOKEN_EOF)) {
    ParseTree = Statement();
    if (ParseTree == NULL) {
      printf("[%s:%d] Parse() returned NULL. ParseTree could not be created.\n", __FILE__, __LINE__);
      return;
    }

    printf("\n[AST]\n");
    PrintAST(ParseTree);
  }
}

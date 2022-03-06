#include "9cc.h"

// ローカル変数
LVar *locals = NULL;

Node *new_node(NodeKind kind, Node *lhs, Node *rhs)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

// 変数を名前で検索する。見つからなかった場合はNULLを返す。
LVar *find_vlar(Token *tok)
{
  for (LVar *var = locals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) return var;
  return NULL;
}

void program();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// program = stmt*
void program()
{
    int i = 0;
    while (!at_eof()) code[i++] = stmt();
    code[i] = NULL;
    return;
}

// stmt = expr ";" | "return" expr ";"
Node *stmt()
{
    Node *node;
    if (consume_return())
    {
      node = calloc(1, sizeof(Node));
      node->kind = ND_RETURN;
      node->lhs = expr();
    }
    else
    {
      node = expr();
    }
    if (!consume(";")) error_at(token->str, "';'ではないトークンです");
    return node;
}

// expr = assign
Node *expr()
{
  Node *node = assign();
  return node;
}

// assign = equality ("=" assign)?
Node *assign()
{
    Node *node = equality();
    if (consume("=")) node = new_node(ND_ASSIGN, node, assign());
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality()
{
  Node *node = relational();

  for (;;)
  {
    if (consume("==")) node = new_node(ND_EQ, node, relational());
    else if (consume("!=")) node = new_node(ND_NE, node, relational());
    else return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational()
{
  Node *node = add();

  for (;;)
  {
    if (consume("<")) node = new_node(ND_LT, node, add());
    else if (consume("<=")) node = new_node(ND_LE, node, add());
    else if (consume(">")) node = new_node(ND_LT, add(), node);
    else if (consume(">=")) node = new_node(ND_LE, add(), node);
    else return node;
  }
}

// add = mul ("+" mul | "-" mul)*
Node *add()
{
  Node *node = mul();

  for (;;)
  {
    if (consume("+")) node = new_node(ND_ADD, node, mul());
    else if (consume("-")) node = new_node(ND_SUB, node, mul());
    else return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul()
{
  Node *node = unary();
  for (;;)
  {
    if (consume("*")) node = new_node(ND_MUL, node, unary());
    else if (consume("/")) node = new_node(ND_DIV, node, unary());
    else return node;
  }
}

// unary = ("+"|"-")? primary
Node *unary()
{
  if (consume("+")) return primary();
  if (consume("-")) return new_node(ND_SUB, new_node_num(0), primary());
  return primary();
}

// primary = num | ident | "(" expr ")"
Node *primary()
{
  // 次のトークンが"("なら、"(" expr ")"のはず
  if (consume("("))
  {
    Node *node = expr();
    expect(")");
    return node;
  }

  // ローカル変数
  Token *tok = consume_ident();
  if (tok)
  {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    
    LVar *lvar = find_vlar(tok);
    if (lvar) 
    {
      node->offset = lvar->offset;
    }
    else
    {
      lvar = calloc(1, sizeof(LVar));
      lvar->next = locals;
      lvar->name = tok->str;
      lvar->len = tok->len;
      if (locals) lvar->offset = locals->offset + 8;
      else lvar->offset = 8;
      node->offset = lvar->offset;
      locals = lvar;
    }
    return node;
  }

  // 上記のいずれでもなければ数値のはず
  return new_node_num(expect_number());
}

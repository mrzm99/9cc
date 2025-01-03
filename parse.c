#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"

/*--------------------------------------------------------------------*/
/*! @brief  制御ブロック
 */
typedef struct {
    Token_t *token;     //!< 現在着目しているトークン
    char *user_input;   //!< 入力プログラム
    Node_t *code[100];  //!< 各statementに対する抽象構文木
    LVar_t *locals;     //!< ローカル変数リスト
} ctrl_blk_9cc_t;

static ctrl_blk_9cc_t ctrl_blk_9cc;
#define get_myself()    (&ctrl_blk_9cc)

/*--------------------------------------------------------------------*/
/*! @brief  プロトタイプ宣言
 */
static Node_t *expr(void);

/*--------------------------------------------------------------------*/
/*! @brief  エラー報告関数
 */
void error(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

/*--------------------------------------------------------------------*/
/*! @brief  エラー箇所表示関数
 */
static void error_at(char *loc, char *fmt, ...)
{
    ctrl_blk_9cc_t *this = get_myself();

    va_list ap;
    va_start(ap, fmt);

    int pos = loc - this->user_input;
    fprintf(stderr, "%s\n", this->user_input);
    fprintf(stderr, "%*s", pos, " ");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

/*--------------------------------------------------------------------*/
/*! @brief  次のトークンが期待している記号の場合、トークンを1つ読み進めて
 *          真を返す。それ以外の場合、偽を返す。
 */
static bool consume(char *op)
{
    ctrl_blk_9cc_t *this = get_myself();

    if (strlen(op) != this->token->len) {
        return false;
    }

    if ((this->token->kind != TK_RESERVED) 
    || (memcmp(this->token->str, op, this->token->len))) {
        return false;
    }
    this->token = this->token->next;

    return true;
}

/*--------------------------------------------------------------------*/
/*! @brief  次のトークンが期待している記号の場合、トークンを1つ読み進めて
 *          真を返す。それ以外の場合、エラーを報告する。
 */
static void expect(char *op)
{
    ctrl_blk_9cc_t *this = get_myself();

    if ((this->token->kind != TK_RESERVED) 
    || (memcmp(this->token->str, op, this->token->len))) {
        error_at(this->token->str, "'%s'ではありません", op);
    }
    this->token = this->token->next;
}

/*--------------------------------------------------------------------*/
/*! @brief  次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
 */
static int expect_number(void)
{
    ctrl_blk_9cc_t *this = get_myself();

    if (this->token->kind != TK_NUM) {
        error_at(this->token->str, "数ではありません");
    }
    int val = this->token->val;
    this->token = this->token->next;

    return val;
}

/*--------------------------------------------------------------------*/
/*! @brief  最終トークンであれば真を返す。最終でなければ偽を返す。
 */
static bool at_eof(void)
{
    ctrl_blk_9cc_t *this = get_myself();

    return this->token->kind == TK_EOF;
}

/*--------------------------------------------------------------------*/
/*! @brief  新しいトークンを作成してcurrに繋げる。
 */
static Token_t *new_token(TokenKind_t kind, Token_t *curr, char *str)
{
    Token_t *token = calloc(1, sizeof(Token_t));
    token->kind = kind;
    token->str = str;
    curr->next = token;

    return token;
}

/*--------------------------------------------------------------------*/
/*! @brief  ローカル変数作成
 */
static Token_t *new_local_var(Token_t *curr, char *str)
{   
    ctrl_blk_9cc_t *this = get_myself();

    // 変数名の長さを取得
    int len = 0;
    int ofs = 0;
    while (1) {
        if (('a' <= *(str + ofs)) && (*(str + ofs) <= 'z')) {
            len++;
            ofs++;
        } else {
            break;
        }
    }
    Token_t *token = new_token(TK_IDENT, curr, str);
    token->len = len;

    return token;
}

/*--------------------------------------------------------------------*/
/*! @brief  入力文字列pをトークナイズして返す。
 */
void tokenize(char *p)
{   
    ctrl_blk_9cc_t *this = get_myself();
    Token_t head;
    head.next = NULL;
    Token_t *curr = &head;

    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        // 変数(a~z)であればトークン作成
        if (('a' <= *p) && (*p <= 'z')) {
            curr = new_local_var(curr, p);
            p = p + curr->len;
            continue;
        }

        // 文字であればトークン作成
        if (((*p == '<') && (*(p+1) == '='))
        ||  ((*p == '>') && (*(p+1) == '='))
        ||  ((*p == '=') && (*(p+1) == '='))
        ||  ((*p == '!') && (*(p+1) == '='))) {
            curr = new_token(TK_RESERVED, curr, p);
            curr->len = 2;
            p += 2;
            continue;
        }
        if ((*p == '+') || (*p == '-') || (*p == '*') || (*p == '/') 
        ||  (*p == '(') || (*p == ')')
        ||  (*p == '<') || (*p == '>')
        ||  (*p == ';') || (*p == '=')) {
            curr = new_token(TK_RESERVED, curr, p);
            curr->len = 1;
            p++;
            continue;
        }

        // 整数であればトークン生成
        if (isdigit(*p)) {
            curr = new_token(TK_NUM, curr, p);
            curr->val = strtol(p, &p, 10);
            continue;
        }

        error("トークナイズできません。");
    }
    // 最終トークンを作成
    new_token(TK_EOF, curr, p);

    // トークナイズ結果を記憶
    this->token = head.next;
}

/*--------------------------------------------------------------------*/
/*! @brief  ノード生成関数
 */
static Node_t *new_node(NodeKind_t kind, Node_t *lhs, Node_t *rhs)
{
    Node_t *node = calloc(1, sizeof(Node_t));
    
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;

    return node;
}

/*--------------------------------------------------------------------*/
/*! @brief  ノード生成関数(整数)
 */
static Node_t *new_node_num(NodeKind_t val)
{
    Node_t *node = calloc(1, sizeof(Node_t));

    node->kind = ND_NUM;
    node->val = val;

    return node;
}

/*--------------------------------------------------------------------*/
/*! @brief  consume_ident関数
 */
static Token_t *consume_ident(void)
{
    ctrl_blk_9cc_t *this = get_myself();
    Token_t *token;

    if (this->token->kind != TK_IDENT) {
        return NULL;
    }
    token = this->token;
    this->token = this->token->next;

    return token;
}

/*--------------------------------------------------------------------*/
/*! @brief  ローカル変数の検索
 */
static LVar_t *find_lvar(Token_t *token)
{
    ctrl_blk_9cc_t *this = get_myself();

    for (LVar_t *var = this->locals; var; var = var->next) {
        if ((var->len == token->len) && (!memcmp(token->str, var->name, var->len))) {
            return var;
        }
    }

    return NULL;
}

/*--------------------------------------------------------------------*/
/*! @brief  primary関数
 */
static Node_t *primary(void)
{   
    ctrl_blk_9cc_t *this = get_myself();

    // 次のトークンが"("なら、"(" expr ")"のはず
    if (consume("(")) {
        Node_t *node = expr();
        expect(")");
        return node;
    }

    // 次のトークンが変数ならノード生成
    Token_t *token = consume_ident();
    if (token) {
        Node_t *node = calloc(1, sizeof(Node_t));
        node->kind = ND_LVAR;

        LVar_t *lvar = find_lvar(token);
        if (lvar) {
            node->offset = lvar->offset;
        } else {
            lvar = calloc(1, sizeof(LVar_t));
            lvar->next = this->locals;
            lvar->name = token->str;
            lvar->len = token->len;
            if (this->locals) {
                lvar->offset = this->locals->offset + 8;
            } else {
                lvar->offset = 8;
            }
            node->offset = lvar->offset;
            this->locals = lvar;
        }
        return node;
    }

    // そうでなければ数値のはず
    return new_node_num(expect_number());
}

/*--------------------------------------------------------------------*/
/*! @brief  unary関数
 */
static Node_t *unary(void)
{
    if (consume("+")) {
        return primary();
    }
    if (consume("-")) {
        return new_node(ND_SUB, new_node_num(0), primary());
    }

    return primary();
}

/*--------------------------------------------------------------------*/
/*! @brief  mul関数
 */
static Node_t *mul(void)
{
    Node_t *node = unary();

    for (;;) {
        if (consume("*")) {
            node = new_node(ND_MUL, node, unary());
        } else if (consume("/")) {
            node = new_node(ND_DIV, node, unary());
        } else {
            return node;
        }
    }
}

/*--------------------------------------------------------------------*/
/*! @brief  add関数
 */
static Node_t *add(void)
{
    Node_t *node = mul();

    for (;;) {
        if (consume("+")) {
            node = new_node(ND_ADD, node, mul());
        } else if (consume("-")) {
            node = new_node(ND_SUB, node, mul());
        } else {
            return node;
        }
    }
}

/*--------------------------------------------------------------------*/
/*! @brief  relational関数
 */
static Node_t *relational(void)
{
    Node_t *node = add();

    for (;;) {
        if (consume("<")) {
            node = new_node(ND_LSS, node, add());
        // '>'は左右ノードを入れ替えて'<'として対応する
        } else if (consume(">")) {
            node = new_node(ND_LSS, add(), node);
        } else if (consume("<=")) {
            node = new_node(ND_LSS_EQU, node, add());
        // ">="は左右ノードを入れ替えて"<="として対応する
        } else if (consume(">=")) {
            node = new_node(ND_LSS_EQU, add(), node);
        } else {
            return node;
        }
    }
}

/*--------------------------------------------------------------------*/
/*! @brief  equality関数
 */
static Node_t *equality(void)
{
    Node_t *node = relational();

    for (;;) {
        if (consume("==")) {
            node = new_node(ND_EQU, node, relational());
        } else if (consume("!=")) {
            node = new_node(ND_NOT_EQU, node, relational());
        } else {
            return node;
        }
    }
}

/*--------------------------------------------------------------------*/
/*! @brief  assign関数
 */
static Node_t *assign(void)
{
    Node_t *node = equality();
    if (consume("=")) {
        node = new_node(ND_ASSIGN, node, assign());
    }
    return node;
}

/*--------------------------------------------------------------------*/
/*! @brief  expr関数
 */
static Node_t *expr(void)
{
    return assign();
}

/*--------------------------------------------------------------------*/
/*! @brief  stmt関数
 */
static Node_t *stmt(void)
{
    Node_t *node = expr();
    expect(";");

    return node;
}

/*--------------------------------------------------------------------*/
/*! @brief  program関数
 */
void program(void)
{
    ctrl_blk_9cc_t *this = get_myself();
    int i = 0;
    
    while (!at_eof()) {
        this->code[i] = stmt();
        i++;
    }
    this->code[i] = NULL;
}

/*--------------------------------------------------------------------*/
/*! @brief code取得（parse.cのグローバル変数をmain.cで直接アクセスしないように用意しているが微妙な気がする。。。）
 */
void get_code(Node_t *p[100])
{
    ctrl_blk_9cc_t *this = get_myself();
    memcpy(p, this->code, sizeof(Node_t*) * 100);
}

/*--------------------------------------------------------------------*/
/*! @brief  初期化
 */
void parser_init(char *p)
{
    ctrl_blk_9cc_t *this = get_myself();

    memset(this, 0, sizeof(ctrl_blk_9cc_t));
    this->user_input = p;
}
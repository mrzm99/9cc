#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*--------------------------------------------------------------------*/
/*! @brief  トークンの種類
 */
typedef enum {
    TK_RESERVED = 0,    //!< 記号
    TK_NUM,             //!< 整数トークン
    TK_EOF,             //!< 入力の終わりを表すトークン
} TokenKind_t;

/*--------------------------------------------------------------------*/
/*! @brief  トークン型
 */
typedef struct Token_ {
    TokenKind_t kind;       //!< トークンの型
    struct Token_ *next;    //!< 次の入力トークン
    int val;                //!< kindがTK_NUMの場合、その数値
    char *str;              //!< トークン文字列
} Token_t;

/*--------------------------------------------------------------------*/
/*! @brief  抽象構文木のノードの種類
 */
typedef enum {
    ND_ADD = 0,     //!< +
    ND_SUB,         //!< -
    ND_MUL,         //!< *
    ND_DIV,         //!< /
    ND_NUM,         //!< 整数 
} NodeKind_t;

/*--------------------------------------------------------------------*/
/*! @brief  抽象構文木のノードの型
 */
typedef struct Node_ {
    NodeKind_t kind;    //!< ノードの型
    struct Node_ *lhs;  //!< 左辺
    struct Node_ *rhs;  //!< 右辺
    int val;            //!< kindがND_NUMの場合のみ使用する
} Node_t;

/*--------------------------------------------------------------------*/
/*! @brief  制御ブロック(グローバル変数)
 */
typedef struct {
    Token_t *token;     //!< 現在着目しているトークン
    char *user_input;   //!< 入力プログラム
} ctrl_blk_9cc_t;

static ctrl_blk_9cc_t ctrl_blk_9cc;
#define get_myself()    (&ctrl_blk_9cc)

/*--------------------------------------------------------------------*/
/*! @brief  プロトタイプ
 */
static Node_t *expr(void);
static Node_t *mul(void);
static Node_t *primary(void);

/*--------------------------------------------------------------------*/
/*! @brief  エラー報告関数
 */
static void error(char *fmt, ...)
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
static bool consume(char op)
{
    ctrl_blk_9cc_t *this = get_myself();

    if ((this->token->kind != TK_RESERVED) || (this->token->str[0] != op)) {
        return false;
    }
    this->token = this->token->next;

    return true;
}

/*--------------------------------------------------------------------*/
/*! @brief  次のトークンが期待している記号の場合、トークンを1つ読み進めて
 *          真を返す。それ以外の場合、エラーを報告する。
 */
static void expect(char op)
{
    ctrl_blk_9cc_t *this = get_myself();

    if ((this->token->kind != TK_RESERVED) || (this->token->str[0] != op)) {
        //- error("'%c'ではありません", op);
        error_at(this->token->str, "'%c'ではありません", op);
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
        //- error("数ではありません");
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
/*! @brief  入力文字列pをトークナイズして返す。
 */
static Token_t *tokenize(char *p)
{
    Token_t head;
    head.next = NULL;
    Token_t *curr = &head;

    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        // 文字であればトークン作成
        if ((*p == '+') || (*p == '-') || (*p == '*') || (*p == '/') 
        ||  (*p == '(') || (*p == ')')) {
            curr = new_token(TK_RESERVED, curr, p);
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

    return head.next;
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
/*! @brief  primary関数
 */
static Node_t *primary(void)
{
    // 次のトークンが"("なら、"(" expr ")"のはず
    if (consume('(')) {
        Node_t *node = expr();
        expect(')');
        return node;
    }

    // そうでなければ数値のはず
    return new_node_num(expect_number());
}
/*--------------------------------------------------------------------*/
/*! @brief  mul関数
 */
static Node_t *mul(void)
{
    Node_t *node = primary();

    for (;;) {
        if (consume('*')) {
            node = new_node(ND_MUL, node, primary());
        } else if (consume('/')) {
            node = new_node(ND_DIV, node, primary());
        } else {
            return node;
        }
    }
}
/*--------------------------------------------------------------------*/
/*! @brief  expr関数
 */
static Node_t *expr(void)
{
    Node_t *node = mul();

    for (;;) {
        if (consume('+')) {
            node = new_node(ND_ADD, node, mul());
        } else if (consume('-')) {
            node = new_node(ND_SUB, node, mul());
        } else {
            return node;
        }
    }   
}

/*--------------------------------------------------------------------*/
/*! @brief  gen関数
 */
static void gen(Node_t *node)
{   
    // 整数（終端ノード）
    if (node->kind == ND_NUM) {
        printf("    push %d\n", node->val);
        return;
    }

    // 左辺と右辺を処理
    gen(node->lhs);
    gen(node->rhs);

    // オペランドをpop
    printf("    pop rdi\n");
    printf("    pop rax\n");

    // 演算子に合わせてアセンブリ生成
    switch(node->kind) {
    case ND_ADD:
        printf("    add rax, rdi\n");
        break;
    case ND_SUB:
        printf("    sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("    imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("    cqo\n");
        printf("    idiv rdi\n");
        break;
    default:
        break;
    }

    printf("    push rax\n");
}
/*--------------------------------------------------------------------*/
/*! @brief  main関数
 */
int main(int argc, char **argv)
{
    ctrl_blk_9cc_t *this = get_myself();

    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    // 制御ブロック初期化
    memset(this, 0, sizeof(ctrl_blk_9cc_t));
    this->user_input = argv[1];

   // トークナイズしてパースする
    this->token = tokenize(this->user_input);
    Node_t *node = expr();

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // 抽象構文木を下りながらコード生成
    gen(node);

    // スタックトップに式全体の値が残っているはずなので
    // それをRAXにロードして関数からの戻り値とする
    printf("    pop rax\n");
    printf("    ret\n");

    return 0;
}
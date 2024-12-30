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
    TK_RESERVED,    //!< 記号
    TK_NUM,         //!< 整数トークン
    TK_EOF,         //!< 入力の終わりを表すトークン
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
/*! @brief  制御ブロック(グローバル変数)
 */
typedef struct {
    Token_t *token;     //!< 現在着目しているトークン
    char *user_input;   //!< 入力プログラム
} ctrl_blk_9cc_t;

static ctrl_blk_9cc_t ctrl_blk_9cc;
#define get_myself()    (&ctrl_blk_9cc)

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
        if ((*p == '+') || (*p == '-')) {
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

   // トークナイズする
    this->token = tokenize(argv[1]);

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // 式の最初は数でなければならないので、チェックして最初のmov命令を出力
    printf("    mov rax, %d\n", expect_number());

    // '+ <数>'あるいは'- <数>'というトークンの並びを消費しつつアセンブリを出力
    while (!at_eof()) {
        if (consume('+')) {
            printf("    add rax, %d\n", expect_number());
            continue;
        }

        expect('-');
        printf("    sub rax, %d\n", expect_number());
    }

    printf("    ret\n");
    return 0;
}
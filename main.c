#include <stdio.h>
#include "9cc.h"

/*--------------------------------------------------------------------*/
/*! @brief  main関数
 */
int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    // 入力文字列取得
    char *user_input = argv[1];

    // トークナイズしてパースする
    Token_t *token = tokenize(user_input);
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
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

    // パーサを初期化
    parser_init(user_input);

    // トークナイズしてパースする
    tokenize(user_input);
    program();

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // プロローグ
    // 変数26個分の領域を確保する
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, 208\n"); // 8byte * 26個(a~z)

    // 先頭の式から順にコード生成
    Node_t *code[100];
    get_code(code);

    for (int i = 0; code[i]; i++) {
        gen(code[i]);

        // 式の評価結果としてスタックに1つの値が残っているはずなので
        // スタックが溢れないようにポップしておく
        printf("    pop rax\n");
    }

    // エピローグ
    // 最後の式の結果がRAXに残っているのでそれが戻り値になる
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");

    return 0;
}
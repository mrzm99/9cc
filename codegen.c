#include <stdio.h>
#include "9cc.h"

/*--------------------------------------------------------------------*/
/*! @brief  gen関数
 */
void gen(Node_t *node)
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
    case ND_EQU:
        printf("    cmp rax, rdi\n");
        printf("    sete al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_NOT_EQU:
        printf("    cmp rax, rdi\n");
        printf("    setne al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LSS:
        printf("    cmp rax, rdi\n");
        printf("    setl al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LSS_EQU:
        printf("    cmp rax, rdi\n");
        printf("    setle al\n");
        printf("    movzb rax, al\n");
        break;
    default:
        break;
    }

    printf("    push rax\n");
}
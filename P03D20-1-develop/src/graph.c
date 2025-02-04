#include "main.h"

int main() {
    printf("Для использования унарного минуса примените \"~\";\n");
    Expression mother;
    char inputExpr[MAX_EXPR_SIZE];

    input_expression(inputExpr, sizeof(inputExpr));
    init_mother(&mother, inputExpr);

    draw_graph(mother.infixExpr);

    return 0;
}
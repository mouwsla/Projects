#include "main.h"

void init_stack(Stack* stack) { stack->top = -1; }

int is_stack_empty(Stack* stack) { return stack->top == -1; }

int is_stack_full(Stack* stack) { return stack->top == MAX_STACK_SIZE - 1; }

void push(Stack* stack, const char* item) {
    if (!is_stack_full(stack)) {
        strcpy(stack->items[++stack->top], item);
    }
}

char* pop(Stack* stack) {
    if (!is_stack_empty(stack)) {
        return stack->items[stack->top--];
    }
    return NULL;
}

char* peek(Stack* stack) {
    if (!is_stack_empty(stack)) {
        return stack->items[stack->top];
    }
    return NULL;
}

void init_num_stack(NumStack* stack) { stack->top = -1; }

int is_num_stack_empty(NumStack* stack) { return stack->top == -1; }

int is_num_stack_full(NumStack* stack) { return stack->top == MAX_STACK_SIZE - 1; }

void push_num(NumStack* stack, double item) {
    if (!is_num_stack_full(stack)) {
        stack->items[++stack->top] = item;
    }
}

double pop_num(NumStack* stack) {
    if (!is_num_stack_empty(stack)) {
        return stack->items[stack->top--];
    }
    return 0;
}

int is_digit(char c) { return c >= '0' && c <= '9'; }

int is_operator(char c) { return strchr("+-*/^~", c) != NULL; }

int is_function(const char* func) {
    return strcmp(func, "sin") == 0 || strcmp(func, "cos") == 0 || strcmp(func, "tan") == 0 ||
           strcmp(func, "ctg") == 0 || strcmp(func, "sqrt") == 0 || strcmp(func, "ln") == 0;
}

int get_operation_priority(const char* op) {
    if (strcmp(op, "(") == 0) {
        return 0;
    }
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) {
        return 1;
    }
    if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
        return 2;
    }
    if (strcmp(op, "^") == 0) {
        return 3;
    }
    if (strcmp(op, "~") == 0) {
        return 4;
    }
    if (is_function(op)) {
        return 5;
    }
    return -1;
}
void get_function(const char* expr, int* pos, char* func) {
    int index = 0;
    while (expr[*pos] != '\0' && !is_digit(expr[*pos]) && !is_operator(expr[*pos]) && expr[*pos] != ' ' &&
           expr[*pos] != '(' && expr[*pos] != ')') {
        func[index++] = expr[*pos];
        (*pos)++;
    }
    func[index] = '\0';
}

void get_string_number(const char* expr, int* pos, char* strNumber) {
    int index = 0;
    strNumber[0] = '\0';

    while (expr[*pos] != '\0' && (is_digit(expr[*pos]) || expr[*pos] == '.')) {
        strNumber[index++] = expr[*pos];
        strNumber[index] = '\0';
        (*pos)++;
    }
    (*pos)--;
}

void to_postfix(const char* infixExpr, char* postfixExpr) {
    Stack stack;
    init_stack(&stack);
    int i = 0, j = 0;
    char strNumber[100];
    char funcName[10];
    while (infixExpr[i] != '\0') {
        char c = infixExpr[i];
        if (c == ' ') {
            i++;
            continue;
        }
        if (is_digit(c) || c == '.') {
            get_string_number(infixExpr, &i, strNumber);
            strcpy(&postfixExpr[j], strNumber);
            j += strlen(strNumber);
            postfixExpr[j++] = ' ';
        } else if (!is_digit(c) && !is_operator(c) && c != '(' && c != ')') {
            get_function(infixExpr, &i, funcName);
            push(&stack, funcName);
            i--;
        } else if (c == '(') {
            push(&stack, "(");
        } else if (c == ')') {
            while (!is_stack_empty(&stack) && strcmp(peek(&stack), "(") != 0) {
                strcpy(&postfixExpr[j], pop(&stack));
                j += strlen(postfixExpr + j);
                postfixExpr[j++] = ' ';
            }
            if (!is_stack_empty(&stack) && strcmp(peek(&stack), "(") == 0) {
                pop(&stack);
            }
            if (!is_stack_empty(&stack) && is_function(peek(&stack))) {
                strcpy(&postfixExpr[j], pop(&stack));
                j += strlen(postfixExpr + j);
                postfixExpr[j++] = ' ';
            }
        } else if (is_operator(c)) {
            char op[2] = {c, '\0'};
            while (!is_stack_empty(&stack) &&
                   get_operation_priority(peek(&stack)) >= get_operation_priority(op)) {
                strcpy(&postfixExpr[j], pop(&stack));
                j += strlen(postfixExpr + j);
                postfixExpr[j++] = ' ';
            }
            push(&stack, op);
        }
        i++;
    }
    while (!is_stack_empty(&stack)) {
        strcpy(&postfixExpr[j], pop(&stack));
        j += strlen(postfixExpr + j);
        postfixExpr[j++] = ' ';
    }
    postfixExpr[j] = '\0';
}

double execute(char op, double first, double second) {
    switch (op) {
        case '+':
            return first + second;
        case '-':
            return first - second;
        case '*':
            return first * second;
        case '/': {
            if (second != 0) {
                return first / second;
            }
            fprintf(stderr, "Ошибка: Деление на ноль\n");
            exit(EXIT_FAILURE);
        }
        case '^':
            return pow(first, second);
        case '~':
            return -second;
        default:
            return 0;
    }
}

double evaluate_function(const char* funcName, double value) {
    if (strcmp(funcName, "sin") == 0) {
        return sin(value);
    } else if (strcmp(funcName, "cos") == 0) {
        return cos(value);
    } else if (strcmp(funcName, "tan") == 0) {
        return tan(value);
    } else if (strcmp(funcName, "ctg") == 0) {
        if (tan(value) != 0) {
            return 1 / tan(value);
        }
        fprintf(stderr, "Ошибка: Деление на ноль при вычислении ctg\n");
        exit(EXIT_FAILURE);
    } else if (strcmp(funcName, "sqrt") == 0) {
        if (value >= 0) {
            return sqrt(value);
        }
        fprintf(stderr, "Ошибка: Отрицательный аргумент для sqrt\n");
        exit(EXIT_FAILURE);
    } else if (strcmp(funcName, "ln") == 0) {
        if (value >= 0) {
            return log(value);
        }
        fprintf(stderr, "Ошибка: Неположительный аргумент для ln\n");
        exit(EXIT_FAILURE);
    } else {
        fprintf(stderr, "Ошибка: Неизвестная функция %s\n", funcName);
        exit(EXIT_FAILURE);
    }
}

double evaluate_postfix(const char* postfixExpr) {
    NumStack stack;
    init_num_stack(&stack);

    char exprCopy[MAX_EXPR_SIZE];
    strcpy(exprCopy, postfixExpr);

    char* token = strtok(exprCopy, " ");

    while (token != NULL) {
        if (is_digit(token[0]) || (token[0] == '-' && is_digit(token[1]))) {
            push_num(&stack, atof(token));
        } else if (is_function(token)) {
            double value = pop_num(&stack);
            double result = evaluate_function(token, value);
            push_num(&stack, result);
        } else if (is_operator(token[0])) {
            double second = pop_num(&stack);
            double first = pop_num(&stack);
            double result = execute(token[0], first, second);
            push_num(&stack, result);
        }
        token = strtok(NULL, " ");
    }

    return pop_num(&stack);
}

void input_expression(char* expr, size_t size) {
    printf("Введите выражение: ");
    fgets(expr, size, stdin);
    expr[strcspn(expr, "\n")] = '\0';
}

void init_mother(Expression* mother, const char* expression) {
    strcpy(mother->infixExpr, expression);
    to_postfix(mother->infixExpr, mother->postfixExpr);
}

double evaluate_function_at(const char* postfixExpr, double x) {
    char modifiedExpr[MAX_EXPR_SIZE];
    snprintf(modifiedExpr, sizeof(modifiedExpr), "%s", postfixExpr);

    char* token = strtok(modifiedExpr, " ");
    char tempExpr[MAX_EXPR_SIZE];
    tempExpr[0] = '\0';

    while (token != NULL) {
        if (strcmp(token, "x") == 0) {
            char xStr[50];
            snprintf(xStr, sizeof(xStr), "%.10f", x);
            strcat(tempExpr, xStr);
        } else {
            strcat(tempExpr, token);
        }
        strcat(tempExpr, " ");
        token = strtok(NULL, " ");
    }

    double result = evaluate_postfix(tempExpr);
    return result;
}

void draw_graph(const char* expr) {
    char postfixExpr[MAX_EXPR_SIZE];
    to_postfix(expr, postfixExpr);

    char graph[HEIGHT][WIDTH + 1];

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            graph[i][j] = '.';
        }
        graph[i][WIDTH] = '\0';
    }

    for (double x_pixel = 1e-6; x_pixel < WIDTH; x_pixel++) {
        double x = X_MIN + (X_MAX - X_MIN) * x_pixel / (WIDTH - 1);
        double y = evaluate_function_at(postfixExpr, x);
        int y_pixel = (round)((Y_MAX - y) * (HEIGHT - 1) / (Y_MAX - Y_MIN));
        int index = (round)(x_pixel);

        if (y_pixel >= 0 && y_pixel < HEIGHT) {
            graph[HEIGHT - y_pixel - 1][index] = '*';
        }
    }

    for (int i = 0; i < HEIGHT; i++) {
        printf("%s\n", graph[i]);
    }
}
#ifndef MAIN_H
#define MAIN_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STACK_SIZE 100
#define MAX_EXPR_SIZE 100
#define HEIGHT 25
#define WIDTH 80
#define X_MIN 0.0
#define X_MAX (4 * M_PI)
#define Y_MIN -1.0
#define Y_MAX 1.0

typedef struct {
    char items[MAX_STACK_SIZE][10];
    int top;
} Stack;

typedef struct {
    double items[MAX_STACK_SIZE];
    int top;
} NumStack;

typedef struct {
    char infixExpr[MAX_EXPR_SIZE];
    char postfixExpr[MAX_EXPR_SIZE];
} Expression;

void init_stack(Stack* stack);
int is_stack_empty(Stack* stack);
int is_stack_full(Stack* stack);
void push(Stack* stack, const char* item);
char* pop(Stack* stack);
char* peek(Stack* stack);

void init_num_stack(NumStack* stack);
int is_num_stack_empty(NumStack* stack);
int is_num_stack_full(NumStack* stack);
void push_num(NumStack* stack, double item);
double pop_num(NumStack* stack);

int is_digit(char c);
int is_operator(char c);
int is_function(const char* func);

int get_operation_priority(const char* op);

void get_function(const char* expr, int* pos, char* func);

void get_string_number(const char* expr, int* pos, char* strNumber);

void to_postfix(const char* infixExpr, char* postfixExpr);

double execute(char op, double first, double second);

double evaluate_function(const char* funcName, double value);

double evaluate_postfix(const char* postfixExpr);

void input_expression(char* expr, size_t size);

void init_mother(Expression* mather, const char* expression);

double evaluate_function_at(const char* postfixExpr, double x);

void draw_graph(const char* expr);

#endif
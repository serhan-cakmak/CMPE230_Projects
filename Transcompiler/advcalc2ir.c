#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <inttypes.h>

// The number of operations and operands in a single input line.
#define MAX_LEN 1024
// Maximum number of variables our program can handle in a shell.
#define MAX_VARIABLES 256
// Number of bits in 64-bit integer type in C
#define INT_BITS 32
// A variable can be as long as MAX_VAR_NAME_LENGTH
#define MAX_VAR_NAME_LENGTH 256




int num_variables = 0;
FILE *fpWrite;
FILE *input;
int regNum = 1;
int error = 0;

typedef struct {
    int type; // 0 -> NUM, for rest look BinaryOperators enum
    int value;
    char regName[64];                                                                                       //////////////////////////////////////// increase the size
} StackNode;

StackNode postfixForNodes[256];

typedef struct {
    StackNode nodeData[MAX_LEN];
    int top;
} Stack;

typedef struct {
    char name[MAX_VAR_NAME_LENGTH];
    int val;
} Variable;

Variable variables[MAX_VARIABLES];
// Operation type enum is used in order to indicate the operation type and other token types such as NUM, VAR, REG.
enum {
    XOR,
    LS,
    RS,
    LR,
    RR,
    RSR,
    NOT,
    NUM,
    REG,
    ADD,
    SUB,
    MUL,
    DIV,
    OPEN_P,
    CLOSE_P,
    AND,
    OR,
    MOD,
    VAR,
    TERMINATE,
} OperationTypes;


// function signatures
void pushNode(Stack* stack, StackNode node);
StackNode popNode(Stack *stack);
int precedence(int type);
char* get_int_or_string_as_String(char* start, int flag);
int  get_int( char* start);
void init_var(char* name, int  value);
int  get_var(char* name);
StackNode operate(StackNode* nodeArr);
char* get_string_inside_parenthesis(char* str, int* comma_index, int* space_num);
StackNode eval_func(char* infix, int comma_index, int operation);
void infix_to_postfix(StackNode*, char* infix);
StackNode evaluate_infix(char* infix);
int strip(char* str);
int lookup(char* var);

void pushNode(Stack* stack, StackNode node){
    stack->top++;
    stack->nodeData[stack->top] = node;
}
StackNode popNode(Stack *stack) {
    if (stack->top == -1) {
        error = 1;
        return stack->nodeData[stack->top];
    }

    StackNode node = stack->nodeData[stack->top];
    stack->top--;
    return node;
}

// to get operator precedence
int precedence(int type) {
    switch(type){
        case MUL:
        case DIV:
        case MOD:
            return 7;

        case ADD:
        case SUB:
            return 6;

        case AND:
            return 2;

        case OR:
            return 1;

        default:
            return 0;
    }
}

char* get_int_or_string_as_String(char* start, int flag) {
    int len = 0;
    char str[64];
    char* result;

    if (isdigit(*start)){
        while (isdigit(*start)) {
            str[len] =   *start;
            len++;
            start++;
        }
    }
    else if (isalpha(*start)){
        while (isalpha(*start)) {
            str[len] = *start;
            len++;
            start++;
        }
    }
    str[len] = '\0';

    result = malloc(sizeof(char) * (len + 1));

    strcpy(result, str);
    result[len + 1] = '\0';
    return result;
}

int  get_int( char* start) {
    int len = 0;
    int  num = 0;
    // Loop over each character in the string
    while (isdigit(*start) ) {
        num = num * 10 + (*start - '0');
        len++;
        start++;
    }
    return num;
}

// initializes a variable with given name and value,
// if it already exits in the variable list, update its value
void init_var(char* name, int  value) {
    for (int i = 0; i < num_variables; i++){
        if (strcmp(variables[i].name, name) == 0) {

            variables[i].val = value;
            return;
        }
    }
    strcpy(variables[num_variables].name, name);
    variables[num_variables].val = value;
    num_variables++;
}

// get the value of a variable if its not in array add.
int  get_var(char* name) {

    for (int i = 0; i < num_variables; i++){
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].val;
        }
    }
    init_var(name, 0);
    return 0;
}


//This function is used for translating c code to appropriate llvm intermediate language.
StackNode fileWriter(StackNode a, StackNode b, int operation){
    if (error == 1){
        StackNode dead;
        return dead;
    }
    // Translation of operation to operation name used in llvm language.
    char* op;
    if (operation == ADD){
        op = "add";

    } else if (operation == SUB){
        op = "sub";
    } else if (operation == MUL){
        op = "mul";
    }else if (operation == AND){
        op = "and";
    }else if (operation == OR){
        op = "or";
    }else if (operation == XOR){
        op = "xor";
    }else if (operation == LS){
        op = "shl";
    }else if (operation == RS){
        op = "ashr";
    }else if (operation == RSR){
        op = "lshr";
    }else if (operation == LR){                             // there is no operation doing bit rotation in llvm so we make this operation by calling other functions in the form of (a << b) | (a >> (INT_BITS - b)
        StackNode first = fileWriter(a,b,LS );
        StackNode tmp;
        tmp.type = NUM;
        tmp.value = INT_BITS ;
        StackNode  sub = fileWriter(tmp,b,SUB);
        StackNode second = fileWriter(a, sub, RSR);      //lshr
        return fileWriter(first, second, OR);

    }else if (operation == RR){
        StackNode first = fileWriter(a,b,RSR);                                  ///lshr
        StackNode tmp;
        tmp.type = NUM;
        tmp.value = INT_BITS ;
        StackNode  sub = fileWriter(tmp,b,SUB);
        StackNode second = fileWriter(a, sub, LS);
        return fileWriter(first, second, OR);
    }
    else if (operation == DIV) {
        op = "sdiv";
    }
    else if (operation == MOD){
        op = "urem";
    }
    else if (operation == NOT){              //Since not is an unary operation there is just fake node b coming as the second node.
        StackNode notNode;                 // There is no direct function as bitwise NOT in llvm. So we call xor function with -1 valued second node.
        notNode.type = NUM;
        *notNode.regName = '\0';
        notNode.value = -1;
        StackNode res = fileWriter(a, notNode, XOR);
        return res;
    }

    StackNode final;
    final.value = 0; // why?
    final.type = REG;

    fflush(stdout);
    //The aim of this if blocks are to print appropriate form of regs or integers or variables. For example you cannot call variable while i32 writing in front of it.
    if (a.type == NUM && b.type == NUM ){                                   // They are actually integer
        fprintf(fpWrite, "    %%%d = %s i32 %d, %d\n", regNum,op, a.value, b.value );
    }

    else if (a.type == REG && b.type == REG) {
        fprintf(fpWrite, "    %%%d = %s i32 %%%s, %%%s\n", regNum,op, a.regName, b.regName );
    }

    else if (a.type == NUM && b.type == REG) {
        fprintf(fpWrite, "    %%%d = %s i32 %d, %%%s\n", regNum, op, a.value, b.regName );
    }

    else if (a.type == REG && b.type == NUM) {
        fprintf(fpWrite, "    %%%d = %s i32 %%%s, %d\n", regNum, op, a.regName, b.value );
    }

    else if (a.type == REG && b.type == VAR) {
        fprintf(fpWrite, "    %%%d = load i32, i32* %%%s\n", regNum, b.regName);
        fprintf(fpWrite, "    %%%d = %s i32 %%%s, %%%d\n", regNum + 1, op, a.regName, regNum );
        regNum++;
    }

    else if (a.type == VAR && b.type == REG) {
        fprintf(fpWrite, "    %%%d = load i32, i32* %%%s\n", regNum, a.regName);
        fprintf(fpWrite, "    %%%d = %s i32 %%%d, %%%s\n", regNum + 1, op, regNum, b.regName );
        regNum++;
    }

    else if (a.type == VAR && b.type == NUM ){       //var 1 is actually var, b is int
        fprintf(fpWrite, "    %%%d = load i32, i32* %%%s\n", regNum, a.regName);
        fprintf(fpWrite, "    %%%d = %s i32 %%%d, %d\n", regNum + 1, op, regNum, b.value );
        regNum++;
    }

    else if (a.type == NUM && b.type == VAR ){
        fprintf(fpWrite, "    %%%d = load i32, i32* %%%s\n", regNum, b.regName);
        fprintf(fpWrite, "    %%%d = %s i32 %d, %%%d\n", regNum + 1,op, a.value, regNum );
        regNum++;
    }

    else{
        fprintf(fpWrite, "    %%%d = load i32, i32* %%%s\n", regNum, a.regName);
        regNum++;
        fprintf(fpWrite, "    %%%d = load i32, i32* %%%s\n", regNum, b.regName);
        fprintf(fpWrite, "    %%%d = %s i32 %%%d, %%%d\n", regNum + 1,op, regNum - 1, regNum );
        regNum++;
    }

    sprintf(final.regName, "%d", regNum);       //Regnum is a global variable which shows how many registers are being used in the current state in order to assign registers in a iterative way.
    regNum++;
    return final;
}

// takes node array and makes neccessary ops on it
StackNode operate(StackNode* postfixForNodes) {
    Stack stack = {.top = -1};
    int i = 0;

    while (postfixForNodes[i].type != TERMINATE) {
        fflush(stdout);

        // check if current token is digit
        if (postfixForNodes[i].type == NUM || postfixForNodes[i].type == VAR || postfixForNodes[i].type == REG) {
            pushNode(&stack, postfixForNodes[i]);
        }

            // check if current token is a operator
        else if (postfixForNodes[i].type != NUM && postfixForNodes[i].type != TERMINATE
                 && postfixForNodes[i].type != VAR) {
            StackNode b = popNode(&stack);
            StackNode a = popNode(&stack);
            StackNode res = fileWriter(a, b, postfixForNodes[i].type);
            pushNode(&stack, res);
        }

        else {
            error = 1;
        }
        i++;
    }
    if (stack.top != 0) {
        error = 1;
    }



    fflush(stdout);

    return popNode(&stack);
}

char *get_string_inside_parenthesis(char *str, int *comma_index, int *space_num) {
    char *result = NULL;
    int count = 0;
    int i = 0;
    int start = -1;

    while (str[i] != '(' | str[i] == '\0') {               //to find the number of space chars after function name
        if (!isspace(str[i])) {
            error = 1;
            return "";
        }
        i++;
    }

    *space_num = i;

    while (str[i] != '\0') {

        if (str[i] == '(') {
            if (count == 0) {
                start = i + 1;
            }
            count++;
        }

        else if (str[i] == ')') {
            count--;
            if (count == 0) {
                result = malloc(i - start + 1);
                strncpy(result, str + start, i - start);
                result[i - start] = '\0';
                break;
            }
        }

        else if (str[i] == ',' && count == 1) {
            *comma_index = i - *space_num -1;
        }

        i++;
    }

    // memory leak when closing parantheses is not found, should be fixed now.
    char *result_copy = NULL;
    if (result) {
        result_copy = strdup(result);
        free(result);
        return result_copy;
    }
    error = 1;
    return result_copy;
}

// function to handle functions such as xor, not, lr etc.
StackNode eval_func(char *infix, int comma_index, int operation) {
    StackNode resNode;

    if (operation == NOT){                                                              //since not is a unary function.
        StackNode tmp = evaluate_infix(infix);
        resNode = fileWriter( tmp, resNode , NOT);                                                                          //////////In filewriter dont use b argument.
        return resNode;
    }

    char first_arg[comma_index + 1]; // allocate memory for the first substring
    strncpy(first_arg, infix, comma_index); // copy the first part of the string
    first_arg[comma_index] = '\0'; // add null character to the end

    char second_arg[strlen(infix) - comma_index +1];
    strncpy(second_arg, infix + comma_index + 1, strlen(infix) - comma_index);
    second_arg[strlen(infix) - comma_index] = '\0';
    //we return stacknodes as the information carrier to the upper levels of the recursion.
    StackNode final_res = evaluate_infix(first_arg);
    StackNode final_res2 = evaluate_infix(second_arg);

    resNode = fileWriter(final_res, final_res2, operation);
    return resNode;
}

// function that takes infix and generates postfix
void infix_to_postfix(StackNode* postfixForNodes, char *infix) {
    Stack stack = {.top = -1};
    Stack postfix = {.top = -1};

    int postfix_i = 0;
    int i = 0;
    int count = 0;

    int opFlag =0;      // 0 == inital condition 1 == Flag added last 2 == var or something added

    while (infix[i] != '\0') {
        char c = infix[i];
        int space_num = 0;
        int comma_index = 0;
        char *inside_infix;

        if (isspace(c)) {
            i++;
            continue;
        }

        else if (isalpha(c)) {
            if (opFlag == 2){
                error = 1;
                break;
            }
            opFlag = 2;
            char *res = get_int_or_string_as_String(&infix[i], 0);
            // We look for spesific function names and send them to be calculated.
            if (strcmp(res, "xor") == 0) {
                inside_infix = get_string_inside_parenthesis(&infix[i + 3], &comma_index, &space_num);

                if (error == 1) {
                    break;
                }
                i += space_num;

                if (comma_index == 0) {
                    error = 1;
                    break;
                }

                StackNode funcres = eval_func(inside_infix, comma_index, XOR);

                postfixForNodes[postfix_i] = funcres;
                postfix_i++;

                i += strlen(inside_infix) + 5;                // +5 because of xor() chars.
                continue;
            }

            else if (strcmp(res, "ls") == 0) {
                inside_infix = get_string_inside_parenthesis(&infix[i + 2], &comma_index,
                                                             &space_num);              // +2 since this function name has 2 chars ls
                if (error == 1) {
                    break;
                }
                i += space_num;

                if (comma_index == 0) {
                    error = 1;
                    break;
                }

                StackNode funcres = eval_func(inside_infix, comma_index, LS);

                postfixForNodes[postfix_i] = funcres;
                postfix_i++;
                i += strlen(inside_infix) + 4;
                continue;
            }

            else if (strcmp(res, "rs") == 0){
                inside_infix = get_string_inside_parenthesis(&infix[i + 2], &comma_index,
                                                             &space_num);              // +2 since this function name has 2 chars ls
                if (error == 1) {
                    break;
                }
                i += space_num;

                if (comma_index == 0) {
                    error = 1;
                    break;
                }

                StackNode funcres = eval_func(inside_infix, comma_index, RS);

                postfixForNodes[postfix_i] = funcres;
                postfix_i++;
                i += strlen(inside_infix) + 4;
                continue;
            }

                // shift either left-right, keep either left-most or right-most digit and place it either to tail or head
            else if (strcmp(res, "lr") == 0)    {
                inside_infix = get_string_inside_parenthesis(&infix[i+2], &comma_index, &space_num);
                if (error == 1) {
                    break;
                }
                i += space_num;

                if (comma_index == 0) {
                    error = 1;
                    break;
                }

                StackNode funcres = eval_func(inside_infix, comma_index, LR);
                postfixForNodes[postfix_i] = funcres;
                postfix_i++;
                i += strlen(inside_infix)+4;
                continue;
            }

            else if (strcmp(res, "rr") == 0){
                inside_infix = get_string_inside_parenthesis(&infix[i+2], &comma_index, &space_num);
                if (error == 1) {
                    break;
                }
                i += space_num;

                if (comma_index == 0) {
                    error = 1;
                    break;
                }

                StackNode funcres = eval_func(inside_infix, comma_index, RR);

                postfixForNodes[postfix_i] = funcres;
                postfix_i++;
                i += strlen(inside_infix)+4;
                continue;
            }

            else if (strcmp(res, "not") == 0){
                inside_infix = get_string_inside_parenthesis(&infix[i+3], &comma_index, &space_num);
                if (error == 1) {
                    break;
                }
                i += space_num;

                StackNode funcres = eval_func(inside_infix, comma_index, NOT);

                postfixForNodes[postfix_i] = funcres;
                postfix_i++;

                i += strlen(inside_infix)+5;
                continue;
            }

            else {
                if (lookup(res) ==0){
                    error = 1;
                    break;
                }
                // creating a variable node to transfer the name of the variable.
                StackNode var;
                *var.regName =  '\0';
                strcat(var.regName, res);
                var.type = VAR;
                var.value = 0;

                postfixForNodes[postfix_i] = var;
                postfix_i++;
                //////////////////////////////////////////regnum++; may cause a problem
                /* i++;*/
            }


            i += strlen(res);
            free(res);
        }

        else if (isdigit(c)) {
            if (opFlag ==2){
                error = 1;
                break;
            }
            opFlag = 2;
            char *res = get_int_or_string_as_String(&infix[i], 1);


            StackNode numNode;
            numNode.value = get_int(res);
            *numNode.regName =  '\0';
            numNode.type = NUM;

            postfixForNodes[postfix_i] = numNode;
            postfix_i += 1;
            i += strlen(res);
        }

        else if (infix[i] == '(') {
            count++;
            int x = 0;
            while (infix[i + x] != '\0' && infix[i + x] != ')') {
                x++;
                if (!isspace(infix[i + x])) {
                    break;
                }
            }

            if (infix[i + x] == ')') {
                error = 1;
                break;
            }

            StackNode openPNode;
            openPNode.type = OPEN_P;
            pushNode(&stack, openPNode);

            i++;
        }

        else if (infix[i] == ')') {

            count--;
            if (count < 0) {
                error = 1;
                break;
            }
            while (stack.top >= 0 && stack.nodeData[stack.top].type != OPEN_P) {
                postfixForNodes[postfix_i] = popNode(&stack);
                postfix_i++;
            }

            if (stack.top >= 0 && stack.nodeData[stack.top].type == OPEN_P) {
                popNode(&stack);
            }

            i++;
        }

            // to rearrange operator stack when the current token is an operator
        else if (c == '+' || c == '-' || c == '*' || c == '&' || c == '|' ||
                 c == '/' || c == '%') {
            if (opFlag !=2){                                                    // has to come after var or digit
                error = 1;
                break;
            }
            opFlag = 1;

            StackNode opNode;
            switch (c)
            {
                case '+':
                    opNode.type = ADD;
                    break;

                case '-':
                    opNode.type = SUB;
                    break;

                case '*':
                    opNode.type = MUL;
                    break;

                case '/':
                    opNode.type = DIV;
                    break;

                case '%':
                    opNode.type = MOD;
                    break;

                case '&':
                    opNode.type = AND;
                    break;

                case '|':
                    opNode.type = OR;
                    break;

                default:
                    break;
            }

            while (stack.top >= 0 && precedence(opNode.type) <= precedence(stack.nodeData[stack.top].type)) {
                postfixForNodes[postfix_i] = popNode(&stack);
                postfix_i++;
            }
            pushNode(&stack, opNode);
            i++;
        }

        else {

            error = 1;
            break;
        }

    }

    while (stack.top != -1) {
        if (stack.nodeData[stack.top].type == CLOSE_P) {
            error = 1;
            break;
        }
        postfixForNodes[postfix_i] = popNode(&stack);
        postfix_i++;
    }

    StackNode terminatorNode;
    terminatorNode.type = TERMINATE;
    postfixForNodes[postfix_i] = terminatorNode;
}

// function that takes infix and returns the expected result of the string
StackNode evaluate_infix(char *infix) {
    StackNode dead;

    if (error == 1){
        return dead ;
    }

    StackNode postfixForNodes[256];

    infix_to_postfix(postfixForNodes, infix);

    if (error == 1){
        return dead;
    }

    StackNode res = operate(postfixForNodes);

    return res;
}

void trim(char *name) {
    int len = strlen(name);
    int start = 0, end = len - 1;

    // find index of first non-whitespace character
    while (isspace(name[start])) {
        start++;
    }

    // find index of last non-whitespace character
    while (end >= 0 && isspace(name[end])) {
        end--;
    }

    int i;
    for (i = 0; i <= end - start; i++) {
        if (!isalpha(name[start +i])) {
            error = 1;
        }
        name[i] = name[start + i];
    }

    name[i] = '\0';

}
// Assigning values to the variable names. If variable hasn't come before the allocate memory for it.
void writeAsssigment(char* var, StackNode value){
    if (lookup(var) ==0){
        fprintf(fpWrite, "    %%%s = alloca i32\n",var);
    }

    if (value.type == REG ){
        fprintf(fpWrite, "    store i32 %%%s,i32* %%%s\n", value.regName, var);
    }else if (value.type == VAR) {
        fprintf(fpWrite, "    %%%d = load i32, i32* %%%s\n", regNum, value.regName);
        fprintf(fpWrite, "    store i32 %%%d,i32* %%%s\n", regNum, var);regNum++;
    }
    else{
        fprintf(fpWrite, "    store i32 %d,i32* %%%s\n", value.value, var);
    }

}
//To check if the variable is in the list.
int lookup(char* var){
    for (int i = 0; i < MAX_VARIABLES ; i++){
        if (strcmp(var, variables[i].name) ==0){
            return 1;
        }
    }
    return 0;
}


int main(int argc, char* argv[]) {
    char line[256 + 1] = "";
    /* printf("> ");*/
    StackNode res;
    int i;
    int lineNum = 0;

    char input_filename[64];
    char output_filename[64];
    char *ext_pos;



    strncpy(input_filename, argv[1], 64 - 1);

    ext_pos = strrchr(input_filename, '.');


    // Replace the extension with "ll"
    strncpy(output_filename, input_filename, ext_pos - input_filename);
    strcpy(output_filename + (ext_pos - input_filename), ".ll");

    fpWrite = fopen(output_filename, "w");
    input = fopen(input_filename, "r");

    fprintf(fpWrite, "; ModuleID = 'advcalc2ir' \n");

    fprintf(fpWrite, "declare i32 @printf(i8*, ...)\n \n");
    fprintf(fpWrite, "@print.str = constant [4 x i8] c\"%%d\\0A\\00\"\n \n \n");

    fprintf(fpWrite, "define i32 @main() { \n");
    int status = 0;

    while (fgets(line, sizeof(line), input)) {
        i = 0;
        lineNum++;
        while (line[i] != '\0') {
            if (!isspace(line[i])) {
                break;
            }
            i++;
        }
        if (i == strlen(line)) {
            continue;
        }

        if (strchr(line, '=') == NULL) {                               //it is an expression
            res = evaluate_infix(line);
            if (error == 1) {
                printf( "Error on line %d!\n", lineNum);
                error = 0;
                status = 1;
                continue;
            }

            if (res.type == VAR) {
                fprintf(fpWrite, "    %%%d = load i32, i32* %%%s\n", regNum, res.regName);
                fprintf(fpWrite, "    call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @print.str, i32 0, i32 0), i32 %%%d ) \n", regNum);
                regNum++;
            }

            else if (res.type == NUM) {
                fprintf(fpWrite, "    call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @print.str, i32 0, i32 0), i32 %d ) \n", res.value);
            }

            else if (res.type == REG) {
                fprintf(fpWrite, "    call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @print.str, i32 0, i32 0), i32 %%%s ) \n", res.regName);
            }

            regNum++;
        }

        else {                                                                    // this part is assinging part
            char *var, *expr;
            var = strtok(line, "=");
            trim(var);
            expr = strtok(NULL, "");
            //keyword check
            if (!isalpha(*var) || (strchr(expr, '=') != NULL) || error == 1 ||
                (strcmp("xor", var) == 0) | (strcmp("ls", var) == 0) ||
                (strcmp("rs", var) == 0) | (strcmp("lr", var) == 0) || (strcmp("rr", var) == 0) ||
                (strcmp("not", var) == 0)) {
                error = 0;
                printf( "Error on line %d!\n", lineNum);
                status = 1;
                continue;
            }

            res = evaluate_infix(expr);
            if (error == 1) {
                printf( "Error on line %d!\n", lineNum);
                error = 0;
                status = 1;
                continue;
            }
            writeAsssigment( var, res);
            init_var(var, res.value);
        }
        memset(line, 0, sizeof(line));


    }

    fprintf(fpWrite, "    ret i32 0\n");
    fprintf(fpWrite, "}");

    fclose(input);
    fclose(fpWrite);
    if (status ==1){
        remove(output_filename);
    }

    return 0;
}
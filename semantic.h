#pragma once
#include "ast.h"
#include "ast_types.h"
#include "hash.h"
#include "symbols.h"
#include "debug.h"
#include "semantic_utils.h"

int SemanticErrors = 0;

// TODO: garantir que vetor sempre é usado com indice
// no typecheck, um acesso a vetor tem o tipo como seu tipo, entao se a é um vetor de int e b é um int, entao a[0] + b é valido

void check_and_set_declarations(AST *node);
void check_undeclared(void);
void check_operands(AST *node);
int expression_typecheck(AST *node);
int find_first_datatype(AST *node);
int check_assignments(AST *node);
int check_return(AST *node);
int check_conditional_stmts(AST *node);

void check_and_set_declarations(AST *node)
{
    int i;
    int required_vec_type = 0;
    if (!node)
        return;

    switch (node->type)
    {
    case AST_VAR_DECL_INT:
    case AST_VAR_DECL_CHAR:
    case AST_VAR_DECL_REAL:
    case AST_VAR_DECL_BOOL:
    {
        if (node->symbol)
        {
            if (node->symbol->type != SYMBOL_IDENTIFIER)
            {
                fprintf(stderr, "Semantic error: identifier %s already declared\n", node->symbol->text);
                ++SemanticErrors;
            }

            node->symbol->type = SYMBOL_VARIABLE;
            node->symbol->datatype = ast_type_to_datatype(node->type);
        }

        break;
    }
    case AST_VEC_DECL_INT:
    case AST_VEC_DECL_CHAR:
    case AST_VEC_DECL_REAL:
    case AST_VEC_DECL_BOOL:
    {
        int required_vec_type = 0;
        if (node->type == AST_VEC_DECL_INT)
            required_vec_type = AST_LIT_INT;
        else if (node->type == AST_VEC_DECL_CHAR)
            required_vec_type = AST_LIT_CHAR;
        else if (node->type == AST_VEC_DECL_REAL)
            required_vec_type = AST_LIT_REAL;

        if (node->symbol)
        {
            if (node->symbol->type != SYMBOL_IDENTIFIER)
            {
                fprintf(stderr, "Semantic error: identifier %s already declared\n", node->symbol->text);
                ++SemanticErrors;
            }

            node->symbol->type = SYMBOL_VECTOR;
            node->symbol->datatype = ast_type_to_datatype(node->type);
            node->symbol->is_vector = 1;
            int vec_size = atoi(node->son[0]->symbol->text);
            int initialization_count = 0;

            if (node->son[1] != NULL)
            {
                AST *initialization_item = node->son[1];
                while (initialization_item != NULL)
                {
                    if (!(verify_literal_compatibility(initialization_item->son[0]->type, required_vec_type)))
                    {
                        fprintf(stderr, "Semantic error: vector %s has initialization item with wrong type (expected type %s got %s)\n",
                                node->symbol->text, ast_type_str(required_vec_type),
                                ast_type_str(initialization_item->son[0]->type));
                        ++SemanticErrors;
                    }
                    initialization_item = initialization_item->son[1];
                    ++initialization_count;
                }

                if (initialization_count != vec_size)
                {
                    fprintf(stderr, "Semantic error: vector %s has %d initialization items, but its size is %d\n",
                            node->symbol->text, initialization_count, vec_size);
                    ++SemanticErrors;
                }
            }
        }

        break;
    }
    case AST_FUNC_DECL_INT:
    case AST_FUNC_DECL_CHAR:
    case AST_FUNC_DECL_REAL:
    case AST_FUNC_DECL_BOOL:
    {
        if (node->symbol)
        {
            if (node->symbol->type != SYMBOL_IDENTIFIER)
            {
                fprintf(stderr, "Semantic error: identifier %s already declared\n", node->symbol->text);
                ++SemanticErrors;
            }

            node->symbol->type = SYMBOL_FUNCTION;
            node->symbol->datatype = ast_type_to_datatype(node->type);
            node->symbol->is_function = 1;

            AST *param = node->son[0];
            int count = 0;
            while (param)
            {

                node->symbol->params[count] = ast_type_to_datatype(param->son[0]->type);
                param = param->son[1];
                ++count;
            }

            node->symbol->param_count = count;
        }

        break;
    }

    case AST_PARAM_INT:
    case AST_PARAM_CHAR:
    case AST_PARAM_REAL:
    case AST_PARAM_BOOL:
    {

        if (node->symbol)
        {
            if (node->symbol->type != SYMBOL_IDENTIFIER)
            {
                fprintf(stderr, "Semantic error: identifier %s already declared\n", node->symbol->text);
                ++SemanticErrors;
            }

            node->symbol->type = SYMBOL_PARAMETER;
            node->symbol->datatype = ast_type_to_datatype(node->type);
        }

        break;
    }
    }

    for (i = 0; i < MAX_SONS; i++)
    {
        check_and_set_declarations(node->son[i]);
    }
}

void check_undeclared(void)
{
    SemanticErrors += hash_check_undeclared();
}

void check_operands(AST *node)
{
    int i;

    if (!node)
        return;

    switch (node->type)
    {

    case AST_NESTED_EXPR:
    {
        int datatype = find_first_datatype(node);
        node->result_datatype = datatype;
        break;
    }

    case AST_MUL:
    case AST_DIV:
    case AST_ADD:
    case AST_SUB:
    case AST_LE:
    case AST_GE:
    case AST_EQ:
    case AST_DIF:
    case AST_GT:
    case AST_LT:
    {
        debug_printf("node->type: %s\n", ast_type_str(node->type));
        AST *left_operand = node->son[0];
        AST *right_operand = node->son[1];

        if (left_operand->symbol != NULL && left_operand->symbol->is_vector && left_operand->type != AST_VEC_ACCESS)
        {
            fprintf(stderr, "Semantic error: invalid left operand (vector should be indexed)\n");
            ++SemanticErrors;
        }

        if (right_operand->symbol != NULL && right_operand->symbol->is_vector && right_operand->type != AST_VEC_ACCESS)
        {
            fprintf(stderr, "Semantic error: invalid right operand (vector should be indexed)\n");
            ++SemanticErrors;
        }

        if (left_operand->symbol != NULL && left_operand->symbol->is_function && left_operand->type != AST_FUNC_CALL)
        {
            fprintf(stderr, "Semantic error: invalid left operand (function should be called)\n");
            ++SemanticErrors;
        }

        if (right_operand->symbol != NULL && right_operand->symbol->is_function && right_operand->type != AST_FUNC_CALL)
        {
            fprintf(stderr, "Semantic error: invalid right operand (function should be called)\n");
            ++SemanticErrors;
        }

        int errored = 0;

        if (!(left_operand->type == AST_NESTED_EXPR) && !(left_operand->type == AST_NEG) && !is_numeric(left_operand) && !is_arithmetic(left_operand))
        {
            errored = 1;
            fprintf(stderr, "Semantic error: invalid left operand\n");
            ++SemanticErrors;
        }

        if (!(right_operand->type == AST_NESTED_EXPR) && !(right_operand->type == AST_NEG) && !is_numeric(right_operand) && !is_arithmetic(right_operand))
        {
            errored = 1;
            fprintf(stderr, "Semantic error: invalid right operand\n");
            ++SemanticErrors;
        }

        if (!errored)
        {
            int left_datatype = 0;
            int right_datatype = 0;

            if (left_operand->symbol)
            {
                left_datatype = left_operand->symbol->datatype;
            }

            if (right_operand->symbol)
            {
                right_datatype = right_operand->symbol->datatype;
            }

            if (left_operand->type == AST_NESTED_EXPR)
            {
                left_datatype = find_first_datatype(left_operand);
            }

            if (right_operand->type == AST_NESTED_EXPR)
            {
                right_datatype = find_first_datatype(right_operand);
            }


            if (left_operand->symbol && right_operand->symbol && left_datatype != right_datatype)
            {
                fprintf(stderr, "Semantic error: operands should have same type\n");
                ++SemanticErrors;
            }
        }

        if (!expression_typecheck(node))
        {
            fprintf(stderr, "Semantic error: invalid resulting expression type for %s\n", ast_type_str(node->type));
            ++SemanticErrors;
        }
        else
        {
            node->result_datatype = find_first_datatype(node);
        }

        break;
    }

    case AST_NEG:
    {
        
        AST *operand = node->son[0];

        if (operand->symbol && operand->symbol->is_vector && operand->type != AST_VEC_ACCESS)
        {
            fprintf(stderr, "Semantic error: invalid unary operand (vector should be indexed)\n");
            ++SemanticErrors;
        }

        if (operand->symbol &&operand->symbol->is_function && operand->type != AST_FUNC_CALL)
        {
            fprintf(stderr, "Semantic error: invalid unary operand (function should be called)\n");
            ++SemanticErrors;
        }

        if (!(operand->type == AST_NESTED_EXPR) && !is_numeric(operand) && !is_arithmetic(operand))
        {
            fprintf(stderr, "Semantic error: invalid unary arithmetic/numeric operand\n");
            ++SemanticErrors;
        }

        if (!expression_typecheck(node))
        {
            fprintf(stderr, "Semantic error: invalid resulting expression type for %s\n", ast_type_str(node->type));
            ++SemanticErrors;
        }
        else
        {
            node->result_datatype = find_first_datatype(node);
            if (node->result_datatype == DATATYPE_BOOL) {
                fprintf(stderr, "Semantic error: invalid resulting expression type for %s (got bool, expected numeric-compatible type)\n", ast_type_str(node->type));
                ++SemanticErrors;
            }
        }
        break;
    }

    case AST_AND:
    case AST_OR:
    {
        AST *left_operand = node->son[0];
        AST *right_operand = node->son[1];

        if (left_operand->symbol && left_operand->symbol->is_vector && left_operand->type != AST_VEC_ACCESS)
        {
            fprintf(stderr, "Semantic error: invalid left operand (vector should be indexed)\n");
            ++SemanticErrors;
        }

        if (right_operand->symbol && right_operand->symbol->is_vector && right_operand->type != AST_VEC_ACCESS)
        {
            fprintf(stderr, "Semantic error: invalid right operand (vector should be indexed)\n");
            ++SemanticErrors;
        }

        if (left_operand->symbol && left_operand->symbol->is_function && left_operand->type != AST_FUNC_CALL)
        {
            fprintf(stderr, "Semantic error: invalid left operand (function should be called)\n");
            ++SemanticErrors;
        }

        if (right_operand->symbol && right_operand->symbol->is_function && right_operand->type != AST_FUNC_CALL)
        {
            fprintf(stderr, "Semantic error: invalid right operand (function should be called)\n");
            ++SemanticErrors;
        }

        int errored = 0;

        if (!is_bool(left_operand) && !(left_operand->type == AST_NESTED_EXPR) && !is_logic(left_operand))
        {
            fprintf(stderr, "Semantic error: invalid left operand for %s\n", ast_type_str(node->type));
            ++SemanticErrors;
        }

        if (!is_bool(right_operand) && !(right_operand->type == AST_NESTED_EXPR) && !is_logic(right_operand))
        {
            fprintf(stderr, "Semantic error: invalid right operand for %s\n", ast_type_str(node->type));
            ++SemanticErrors;
        }

        if (!errored)
        {
            int left_datatype = 0;
            int right_datatype = 0;

            if (left_operand->symbol)
            {
                left_datatype = left_operand->symbol->datatype;
            }

            if (right_operand->symbol)
            {
                right_datatype = right_operand->symbol->datatype;
            }

            if (left_operand->type == AST_NESTED_EXPR)
            {
                left_datatype = find_first_datatype(left_operand);
            }

            if (right_operand->type == AST_NESTED_EXPR)
            {
                right_datatype = find_first_datatype(right_operand);
            }


            if (left_operand->symbol && right_operand->symbol && left_datatype != right_datatype)
            {
                fprintf(stderr, "Semantic error: operands should have same type\n");
                ++SemanticErrors;
            }
        }

        if (!expression_typecheck(node))
        {
            fprintf(stderr, "Semantic error: invalid resulting expression type for %s\n", ast_type_str(node->type));
            ++SemanticErrors;
        }
        else
        {
            node->result_datatype = find_first_datatype(node);
        }

        break;
    }

    case AST_NOT:
    {
        AST *operand = node->son[0];

    
        if (operand->symbol && operand->symbol->is_vector && operand->type != AST_VEC_ACCESS)
        {
            fprintf(stderr, "Semantic error: invalid unary logical operand (vector should be indexed)\n");
            ++SemanticErrors;
        }

        if (operand->symbol && operand->symbol->is_function && operand->type != AST_FUNC_CALL)
        {
            fprintf(stderr, "Semantic error: invalid unary logical operand (function should be called)\n");
            ++SemanticErrors;
        }

        if (!(operand->type == AST_NESTED_EXPR) && !is_logic(operand) && !is_bool(operand))
        {
            fprintf(stderr, "Semantic error: invalid unary logical operand (%s)\n", datatype_str[operand->symbol->datatype]);
            ++SemanticErrors;
        }

        if (!expression_typecheck(node))
        {
            fprintf(stderr, "Semantic error: invalid resulting expression type for %s\n", ast_type_str(node->type));
            ++SemanticErrors;
        }
        else
        {
            node->result_datatype = find_first_datatype(node);

            fprintf(stderr, "result datatype: %s\n", datatype_str[node->result_datatype]);
        }

        break;
    }
    }

    for (i = 0; i < MAX_SONS; i++)
    {
        check_operands(node->son[i]);
    }
}

int expression_typecheck(AST *node)
{
    int i;

    if (!node)
        return 0;
    if (node->typechecked)
        return 1;

    if (node->type == AST_NESTED_EXPR)
    {
        return expression_typecheck(node->son[0]);
    }

    if (node->type == AST_IDENTIFIER || node->type == AST_VEC_ACCESS || node->type == AST_FUNC_CALL ||
        node->type == AST_LIT_INT || node->type == AST_LIT_REAL || node->type == AST_LIT_CHAR)
    {

        if (node->symbol)
        {
            node->typechecked = 1;

            if (node->symbol->datatype == DATATYPE_CHAR)
            {
                return DATATYPE_INT;
            }

            if (node->type == AST_LIT_INT)
                return DATATYPE_INT;
            if (node->type == AST_LIT_REAL)
                return DATATYPE_REAL;
            if (node->type == AST_LIT_CHAR)
                return DATATYPE_CHAR;

            return node->symbol->datatype;
        }
    }

    if (is_logic(node))
    {
        if (is_binary(node))
        {
            node->typechecked = 1;

            return ((node->son[0]->type == AST_NESTED_EXPR || is_logic(node->son[0])) && expression_typecheck(node->son[1])) ||
                   ((node->son[1]->type == AST_NESTED_EXPR || is_logic(node->son[1])) && expression_typecheck(node->son[0])) ||
                   (expression_typecheck(node->son[0]) && expression_typecheck(node->son[1]));
        }

        if (is_unary(node))
        {
            node->typechecked = 1;
            return (is_logic(node->son[0]) && expression_typecheck(node->son[0])) ||
                   (expression_typecheck(node->son[0]));
        }
    }


    if (is_binary(node))
    {
        node->typechecked = 1;
        return expression_typecheck(node->son[0]) == expression_typecheck(node->son[1]);
    }

    if (is_unary(node))
    {
        node->typechecked = 1;
        return expression_typecheck(node->son[0]);
    }
}

int find_first_datatype(AST *node)
{
    int i;

    if (!node)
        return 0;

    if (is_logic(node) && node->son[0] && node->son[0]->type != AST_NESTED_EXPR)
    {
        return DATATYPE_BOOL;
    }

    if (node->type == AST_NESTED_EXPR)
    {
        return find_first_datatype(node->son[0]);
    }

    if (node->type == AST_IDENTIFIER || node->type == AST_VEC_ACCESS || node->type == AST_FUNC_CALL ||
        node->type == AST_LIT_INT || node->type == AST_LIT_REAL || node->type == AST_LIT_CHAR)
    {
        if (node->symbol)
        {
            return node->symbol->datatype;
        }
    }

    if (is_binary(node))
    {
        return find_first_datatype(node->son[0]);
    }

    if (is_unary(node))
    {
        return find_first_datatype(node->son[0]);
    }
}

int check_assignments(AST *node)
{
    int i;

    if (!node)
        return 0;

    if (node->type == AST_VAR_ATTRIB)
    {

        int expected_datatype = node->symbol->datatype;
        int resulting_datatype = node->son[0]->result_datatype;

        if (node->son[0]->type == AST_IDENTIFIER)
        {
            if (node->son[0]->symbol->is_vector && node->son[0]->type != AST_VEC_ACCESS)
            {
                fprintf(stderr, "Semantic error: invalid assignment of vector to scalar\n");
                ++SemanticErrors;
            }

            if (node->son[0]->symbol->is_function && node->son[0]->type != AST_FUNC_CALL)
            {
                fprintf(stderr, "Semantic error: invalid assignment of function to scalar\n");
                ++SemanticErrors;
            }
        }

        if (expected_datatype != resulting_datatype && resulting_datatype != 0)
        {
            fprintf(stderr, "Semantic error: invalid assignment of %s to %s\n", datatype_str[resulting_datatype], datatype_str[expected_datatype]);
            ++SemanticErrors;
        }
    }

    if (node->type == AST_VEC_ATTRIB)
    {
        AST *vec_indexer = node->son[0];

        if (vec_indexer->type != AST_LIT_INT && vec_indexer->type != AST_LIT_CHAR)
        {
            if (vec_indexer->type == AST_FUNC_CALL)
            {
                int func_datatype = vec_indexer->symbol->datatype;
                if (func_datatype != DATATYPE_INT && func_datatype != DATATYPE_CHAR)
                {
                    fprintf(stderr, "Semantic error: invalid vector indexer type (expected int or char, got %s -> %s)\n", ast_type_str(vec_indexer->type), datatype_str[func_datatype]);
                    ++SemanticErrors;
                }
            }
            else
            {
                int vec_indexer_result_type = vec_indexer->result_datatype;
                if (vec_indexer_result_type != DATATYPE_INT && vec_indexer_result_type != DATATYPE_CHAR)
                {
                    fprintf(stderr, "Semantic error: invalid vector indexer type (expected int or char, got %s)\n", ast_type_str(node->type));
                    ++SemanticErrors;
                }
            }
        }

        int expected_datatype = node->symbol->datatype;
        int resulting_datatype = node->son[1]->result_datatype;

        if (node->son[1]->type == AST_IDENTIFIER)
        {
            if (node->son[1]->symbol->is_vector && node->son[1]->type != AST_VEC_ACCESS)
            {
                fprintf(stderr, "Semantic error: invalid assignment of vector to vector index\n");
                ++SemanticErrors;
            }

            if (node->son[1]->symbol->is_function && node->son[1]->type != AST_FUNC_CALL)
            {
                fprintf(stderr, "Semantic error: invalid assignment of function to vector index\n");
                ++SemanticErrors;
            }
        }

        if (expected_datatype != resulting_datatype && resulting_datatype != 0)
        {
            fprintf(stderr, "Semantic error: invalid assignment of %s to %s[]\n", datatype_str[resulting_datatype], datatype_str[expected_datatype]);
            ++SemanticErrors;
        }
    }

    for (i = 0; i < MAX_SONS; i++)
    {
        check_assignments(node->son[i]);
    }
}

int check_return(AST *node)
{
    int i;

    if (!node)
        return 0;

    int found = 0;
    static int errored = 0;
    if (is_func_declaration(node) || node->type == AST_BODY)
    {
        AST *body;
        if (node->type == AST_BODY) {
            body = node;
        } else {
            body = node->son[1];
        }
    
        AST *cmd_list =  cmd_list = body->son[0];
           
       
     
        if (cmd_list)
        {
            AST *cmd = cmd_list->son[0];
            while (cmd)
            {

                if (cmd->type == AST_RETURN_CMD)
                {
                    found = 1;
     
                    if (!compare_datatypes(cmd->son[0]->result_datatype, node->symbol->datatype) && !(validate_return_type(node, cmd->son[0])))
                    {
                 
                        if (cmd->son[0]->result_datatype != 0 ||  cmd->son[0]->symbol)
                        {
                            if (cmd->son[0]->result_datatype != 0) {
                                fprintf(stderr, "Semantic error: invalid return type (expected %s, got %s)\n", datatype_str[node->symbol->datatype], datatype_str[cmd->son[0]->result_datatype]);
                                ++SemanticErrors;
                            } else {
                                fprintf(stderr, "Semantic error: invalid return type (expected %s, got %s -> %s)\n", datatype_str[node->symbol->datatype], ast_type_str(cmd->son[0]->type), cmd->son[0]->symbol ? datatype_str[cmd->son[0]->symbol->datatype] : "invalid");
                                ++SemanticErrors;
                            }
                        }
                    }
                }

                if (cmd->type == AST_IF || cmd->type == AST_IF_ELSE || cmd->type == AST_LOOP) {
                    if (cmd->son[1]->type == AST_BODY) {
                        found = check_return(cmd->son[1]);
                    } else {
                        fprintf(stderr, "IF HAS NO BODY!\n");
                    }
                }

                if (cmd->type == AST_BODY) {
                    fprintf(stderr, "BODY!\n");
                }

                if (cmd_list->son[1] != NULL)
                {
                    cmd_list = cmd_list->son[1];
                    if (cmd_list)
                        cmd = cmd_list->son[0];
                }
                else
                {
                    cmd = NULL;
                }
            }
        }

        if (!found && !errored) {
            errored = 1;
        fprintf(stderr, "Semantic error: missing return statement\n");
        ++SemanticErrors;
    }
    }

    for (i = 0; i < MAX_SONS; i++)
    {
        check_return(node->son[i]);
    }

   

    return found;
}

int check_function_call(AST *node)
{
    int i;

    if (!node)
        return 0;

    if (node->type == AST_FUNC_CALL)
    {
        int found = 0;
        int error_in_number_of_params = 0;
        AST *expr_list = node->son[0];
        if (expr_list)
        {
            AST *expr = expr_list->son[0];
            int index = 0;
            while (expr)
            {

                if (node->symbol->params[index] == 0 && !error_in_number_of_params)
                {
                    AST *aux_expr = expr;
                    AST *aux_expr_list = expr_list;
                    while (aux_expr)
                    {
                        if (aux_expr_list->son[1] != NULL)
                        {
                            aux_expr_list = aux_expr_list->son[1];
                            if (aux_expr_list)
                                aux_expr = aux_expr_list->son[0];
                        }
                        else
                        {
                            aux_expr = NULL;
                        }

                        index++;
                    }
                    fprintf(stderr, "Semantic error: invalid number of parameters in function call %s (expected %d, got %d)\n", node->symbol->text, node->symbol->param_count, index);
                    ++SemanticErrors;
                    error_in_number_of_params = 1;
                }

                int expected_datatype = node->symbol->params[index];
                int actual_datatype;

                if (expr->result_datatype != 0)
                {
                    actual_datatype = expr->result_datatype;
                }
                else
                {
                    actual_datatype = expr->symbol->datatype;
                }

                if (!compare_datatypes(expected_datatype, actual_datatype) && expected_datatype != 0)
                {
                    fprintf(stderr, "Semantic error: invalid parameter type (expected %s, got %s)\n", datatype_str[expected_datatype], datatype_str[actual_datatype]);
                    ++SemanticErrors;
                }

                index++;
                if (expr_list->son[1] != NULL)
                {
                    expr_list = expr_list->son[1];
                    if (expr_list)
                        expr = expr_list->son[0];
                }
                else
                {
                    if (node->symbol->params[index] != 0)
                    {
                        fprintf(stderr, "Semantic error: invalid number of parameters in function call %s (expected %d got %d)\n", node->symbol->text, node->symbol->param_count, index);
                        ++SemanticErrors;
                    }
                    expr = NULL;
                }
            }
        }
    }

    for (i = 0; i < MAX_SONS; i++)
    {
        check_function_call(node->son[i]);
    }
}

int check_conditional_stmts(AST *node)
{
    int i;

    if (!node)
        return 0;

    if (node->type == AST_IF || node->type == AST_IF_ELSE || node->type == AST_LOOP)
    {
        if (node->son[0]->result_datatype != DATATYPE_BOOL && (node->son[0]->symbol && node->son[0]->symbol->datatype != DATATYPE_BOOL))
        {
            fprintf(stderr, "Semantic error: invalid conditional statement (expected bool, got %s)\n", datatype_str[node->son[0]->result_datatype]);
            ++SemanticErrors;
        }
    }

    for (i = 0; i < MAX_SONS; i++)
    {
        check_conditional_stmts(node->son[i]);
    }
}
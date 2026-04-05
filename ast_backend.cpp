AstNode *advanceAstNodeAndCheckType(CompilerState *state, AstNode *node, EasyTokenType expectedType) {
    state->currentNode = node->next;
    node = state->currentNode;
    bool correct = false;
    if(node) {
        if(node->token.type == expectedType) {
            correct = true;
        } else {
            state->error = easy_createString_printf(&globalPerFrameArena, "Expected a %s, got a %s", LexTokenTypeStrings[expectedType], LexTokenTypeStrings[node->token.type]);
        }
    } else {
        state->error = "Unexpected end of line. Expected a variable declaration.";
    }
    if(!correct) {
        //NOTE: Return empty node if not correct syntax
        node = 0;
    }
    return node;
}

bool isEndingInstruction(AstNode *node) {
    return (node->operation.type == OP_CODE_CLEAR || node->token.type == TOKEN_CLOSE_PARENTHESIS);
}

AstNode *advanceAstNode(CompilerState *state) {
    state->currentNode = state->currentNode->next;
    AstNode *node = state->currentNode;
    if(!node) {
        state->error = "Unexpected end of line. Expected a variable declaration.";
    }

    return node;
}
AstNode *parseExpression(CompilerState *state, AstNode *node);
void parseVariableAssign(CompilerState *state, AstNode *node) {
    if(node) {
        //NOTE: Check if variable has been declared
        char *name = nullTerminateArena(node->token.at, node->token.size, &globalPerFrameArena);
        AstVariable *variable = getCompilerVariable(state, name);

        if(!variable) {
            printf("Variable not declared. Adding it now.\n");
            pushCompilerVariable(state, name, AST_VARIABLE_NUMBER);
        }

        node = advanceAstNodeAndCheckType(state, node, TOKEN_EQUALS);
        if(node) {
            //NOTE: Add variable as in use
            node = advanceAstNode(state);
            state->currentNode = parseExpression(state, node);
            if(!state->error) {
                VmOperation data = { .type = OP_CODE_VARIABLE_ASSIGN, .name = name};
                pushArrayItem(state->operations, data, VmOperation);

                //NOTE: Also push a variable for the print operation to print the outcome of the variable
                data = { .type = OP_CODE_VARIABLE_REFERENCE, .name = name };
                pushArrayItem(state->operations, data, VmOperation);
            }
        } else {

        }
    }
}

void pushPrintOperation(CompilerState *state) {
    VmOperation data = { .type = OP_CODE_NUMBER, .value_ = (double)(state->calculatorLineAt++) };
    pushArrayItem(state->operations, data, VmOperation);

    data = { .type = OP_CODE_PRINT };
    pushArrayItem(state->operations, data, VmOperation);
}

AstNode *parseExpression(CompilerState *state, AstNode *node) {
    AstNode *postNodeOperation = {};

    while(node && !state->error) {
        bool addedThisLoop = false;

        if(node->child) {
            assert(node->type == AST_TYPE_PARENT);

            if(node->token.type == TOKEN_OPEN_PARENTHESIS) {
                if(!node->next || node->next->token.type != TOKEN_CLOSE_PARENTHESIS) {
                    state->error = "Expected a closed parenthesis.";
                }
            }
            parseExpression(state, node->child);


            // node = node->next;
        } else if(node->type == AST_TYPE_VALUE) {
            VmOperation data = {};
            if(node->operation.type == OP_CODE_NUMBER) {
                data = node->operation;
                pushArrayItem(state->operations, data, VmOperation);
                if(node->next && node->next->type == AST_TYPE_VALUE) {
                    state->error = "Expected an operator";
                }
            } else {
                if(node->next && node->next->token.type == TOKEN_EQUALS) {

                    parseVariableAssign(state, node);
                } else {
                    char *name = nullTerminateArena(node->token.at, node->token.size, &globalPerFrameArena);
                    AstVariable *variable = getCompilerVariable(state, name);

                    if(variable) {
                        data = { .type = OP_CODE_VARIABLE_REFERENCE, .name = name };
                        pushArrayItem(state->operations, data, VmOperation);
                    } else {
                        state->error = "Variable not Defined";
                    }

                    if(node->next && node->next->type == AST_TYPE_VALUE) {
                        state->error = "Expected an operator";
                    }
                }
            }

        } else if(node->type == AST_TYPE_OPERATION || node->type == AST_TYPE_FUNC) {
                //NOTE: Have to push the values on first. Have already pushed the first one becuase it came before
                postNodeOperation = node;
                // DEBUG_lexPrintToken(&node->token);
                addedThisLoop = true;

            if(!isEndingInstruction(node)) {
                if(!node->next) {
                    state->error = "Expected a value";
                } else if(node->next->type == AST_TYPE_OPERATION) {
                    state->error = "Didn't expect an operation.";
                }
            }
        } else {
            if(node->operationType == AST_OPERATION_BEGIN_STATEMENT) {
                pushPrintOperation(state);
            }

        }

        if(!addedThisLoop && postNodeOperation) {
            pushArrayItem(state->operations, postNodeOperation->operation, VmOperation);
            postNodeOperation = 0;
        }

        if(node) {
            node = node->next;
        }

    }

    return node;
}

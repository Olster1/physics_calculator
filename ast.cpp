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

            if(node->operationType == AST_OPERATION_BEGIN_STATEMENT) {
                pushPrintOperation(state);
            }
            
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
                    data = { .type = OP_CODE_VARIABLE_REFERENCE, .name = name };
                    pushArrayItem(state->operations, data, VmOperation);

                    if(node->next && node->next->type == AST_TYPE_VALUE) {
                        state->error = "Expected an operator";
                    }
                }
            }

        } else if(node->type == AST_TYPE_OPERATION) {
                //NOTE: Have to push the values on first. Have already pushed the first one becuase it came before
                postNodeOperation = node;
                DEBUG_lexPrintToken(&node->token);
                addedThisLoop = true;

            if(!isEndingInstruction(node)) {
                if(!node->next) {
                    state->error = "Expected a value";
                } else if(node->next->type == AST_TYPE_OPERATION) {
                    state->error = "Didn't expect an operation.";
                }
            }
        }

        if(!addedThisLoop && postNodeOperation) {
            pushArrayItem(state->operations, postNodeOperation->operation, VmOperation);
            postNodeOperation = 0;
        }

        if(node) {
            node = node->next;
        }

        if(state->error) {
            printf("%s\n", state->error);
        }
    }

    return node;
}


bool compileToByteCode(char *codeToCompile, VmOperation **operations) {
     EasyTokenizer tokenizer = lexBeginParsing(codeToCompile, EASY_LEX_OPTION_EAT_WHITE_SPACE);

    AstTree *tree = pushStruct(&globalPerFrameArena, AstTree);

    CompilerState *state = pushStruct(&globalPerFrameArena, CompilerState);
    //NOTE: First node so just set it
    AstNode *startNode = pushStruct(&globalPerFrameArena, AstNode);
    startNode->type = AST_TYPE_PARENT;
    startNode->operationType = AST_OPERATION_BEGIN_STATEMENT;
    tree->current = tree->start = startNode;

    bool parsing = true;
    EasyToken lastToken = {};
    while(parsing) {
        EasyToken t = lexGetNextToken(&tokenizer);

        if(t.type == TOKEN_NULL_TERMINATOR) {
            parsing = false;
        } else if(t.type == TOKEN_SEMI_COLON) {
            //NOTE: Don't add a semi colon, we're building an 'abstract' syntax tree, just end the statment
            astPopToRoot(tree);
        } else if(t.type == TOKEN_OPEN_BRACKET) {
        } else if(t.type == TOKEN_OPEN_PARENTHESIS) {
            createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_NONE }, getAstTypeForToken(t), getOperationType(t));
        } else if(t.type == TOKEN_CLOSE_PARENTHESIS) {
            createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_NONE }, getAstTypeForToken(t), getOperationType(t));
        } else if(t.type == TOKEN_INTEGER) {
            createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_NUMBER, .value_= (double)t.intVal }, getAstTypeForToken(t), getOperationType(t));
        } else if(t.type == TOKEN_EQUALS) { 
            createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_NONE }, getAstTypeForToken(t), getOperationType(t));
        } else if(t.type == TOKEN_FLOAT) { 
            createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_NUMBER, .value_= (double)t.floatVal }, getAstTypeForToken(t), getOperationType(t));
        } else if(t.type == TOKEN_PLUS) { 
            createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_ADD }, getAstTypeForToken(t), getOperationType(t));
        } else if(t.type == TOKEN_CARROT) { 
            createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_POWER_TO }, getAstTypeForToken(t), getOperationType(t));
        } else if(t.type == TOKEN_MINUS) { 
            //TODO: This doesn't handle 3-- or --3 type operator
            if(lastToken.type != TOKEN_MINUS) {
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_MINUS }, getAstTypeForToken(t), getOperationType(t));
            } else {
                 //NOTE: A minus is actually '-1 *' 
                t = lexInitToken(TOKEN_FLOAT, "-1", 2, t.lineNumber);
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_NUMBER, .value_= -1.0}, getAstTypeForToken(t), getOperationType(t));

                t = lexInitToken(TOKEN_ASTRIX, "*", 1, t.lineNumber);
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_MULTIPLY }, getAstTypeForToken(t), getOperationType(t));
            }
        } else if(t.type == TOKEN_ASTRIX) { 
            createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_MULTIPLY }, getAstTypeForToken(t), getOperationType(t));
        } else if(t.type == TOKEN_FORWARD_SLASH) { 
            createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_DIVIDE }, getAstTypeForToken(t), getOperationType(t));
        } else if(t.type == TOKEN_WORD) { 
            if(easyString_stringsMatch_null_and_count("clear", t.at, t.size)) {
                EasyToken nextToken = lexSeeNextToken(&tokenizer);
                if(nextToken.type != TOKEN_SEMI_COLON) {
                    state->error = "Didn't expect any values. Please try use clear command again without any other values.";
                } else {
                    createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_CLEAR }, getAstTypeForToken(t), getOperationType(t));
                }
            } else if(easyString_stringsMatch_null_and_count("sin", t.at, t.size)) {
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_SIN }, getAstTypeForToken(t), getOperationType(t));
            } else if(easyString_stringsMatch_null_and_count("cos", t.at, t.size)) {
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_COS }, getAstTypeForToken(t), getOperationType(t));
            } else if(easyString_stringsMatch_null_and_count("tan", t.at, t.size)) {
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_TAN }, getAstTypeForToken(t), getOperationType(t));
            } else if(easyString_stringsMatch_null_and_count("asin", t.at, t.size)) {
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_ARCSIN }, getAstTypeForToken(t), getOperationType(t));
            } else if(easyString_stringsMatch_null_and_count("acos", t.at, t.size)) {
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_ARCCOS }, getAstTypeForToken(t), getOperationType(t));
            } else if(easyString_stringsMatch_null_and_count("atan", t.at, t.size)) {
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_ARCTAN }, getAstTypeForToken(t), getOperationType(t));
            } else {
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_VARIABLE_REFERENCE}, getAstTypeForToken(t, false), getOperationType(t));
            }
        }
        lastToken = t;
    }
    
    if(state->error) {
        printf("%s\n", state->error);
    } else {
    
        state->operations = operations;
        state->currentNode = tree->start;

        printAstTree(tree);

        bool running = true;
        //NOTE: Now have ast, walk through this to output the vm instructions
        while(state->currentNode && running) {
            VmOperation op = state->currentNode->operation;
            state->currentNode = parseExpression(state, state->currentNode);            
        }
    }

    return state->error != 0;
}
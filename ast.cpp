AstNode *advanceAstNodeAndCheckType(CompilerState *state, EasyTokenType expectedType) {
    state->currentNode = state->currentNode->next;
    AstNode *node = state->currentNode;
    bool correct = false;
    if(node) {
        if(node->token.type == expectedType) {
            correct = true;
        } else {
            state->error = easy_createString_printf(&globalPerFrameArena, "Expected a word, got a %s", LexTokenTypeStrings[node->token.type]);    
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

AstNode *advanceAstNode(CompilerState *state) {
    state->currentNode = state->currentNode->next;
    AstNode *node = state->currentNode;
    if(!node) {
        state->error = "Unexpected end of line. Expected a variable declaration.";
    }
    
    return node;
}

AstNode *parseExpression(CompilerState *state, AstNode *node) {
    bool parse = true;
    AstNode *postNodeOperation = {};
    
    while(parse) {
        if(node->token.type != TOKEN_SEMI_COLON) {
            if(node->child) {
                parseExpression(state, node->child);
                node = node->next;
            } else if(node->type == AST_TYPE_VALUE) {
                VmOperation data = {};
                if(node->operation.type == OP_CODE_NUMBER) {
                    data = node->operation;
                } else {
                    char *name = nullTerminateArena(node->token.at, node->token.size, &globalPerFrameArena);
                    data = { .type = OP_CODE_VARIABLE_REFERENCE, .name = name };
                }
                pushArrayItem(state->operations, data, VmOperation);
                
            } else if(node->type == AST_TYPE_OPERATION) {
                if(node->operationType == AST_OPERATION_POST) {
                    //NOTE: Have to push the values on first. Have already pushed the first one becuase it came before
                    postNodeOperation = node;
                } else {
                    pushArrayItem(state->operations, node->operation, VmOperation);
                }
                

                if(!node->next) {
                    state->error = "Expected a value";
                } else if(node->next->type == AST_TYPE_OPERATION) {
                    state->error = "Didn't expect an operation.";
                }
            }
        } else {
            parse = false;
        }
        if(postNodeOperation) {
            pushArrayItem(state->operations, postNodeOperation->operation, VmOperation);
            postNodeOperation = 0;
        }

        node = node->next;
    }
    return node;
}

void parseVariableAssign(CompilerState *state) {
    AstNode *node = advanceAstNodeAndCheckType(state, TOKEN_WORD);
    if(node) {
        //NOTE: Check if variable has been declared
        char *name = nullTerminateArena(node->token.at, node->token.size, &globalPerFrameArena);
        AstVariable *variable = getCompilerVariable(state, name);
        if(variable) {
            node = advanceAstNodeAndCheckType(state, TOKEN_EQUALS);
            if(node) {
                //NOTE: Add variable as in use
                node = advanceAstNode(state);
                state->currentNode = parseExpression(state, node);
                if(!state->error) {
                    //NOTE: Now add the byte code for assigning variable
                    VmOperation data = { .type = OP_CODE_STRING, .name = name };
                    pushArrayItem(state->operations, data, VmOperation);

                    data = { .type = OP_CODE_VARIABLE_ASSIGN };
                    pushArrayItem(state->operations, data, VmOperation);
                }
            }
        } else {
            state->error = easy_createString_printf(&globalPerFrameArena, "Variable name: %s not declared.", name);    
        }
    }
}

void parseVariableDeclare(CompilerState *state) {
    if(state->currentNode->next) {
        AstNode *node = advanceAstNodeAndCheckType(state, TOKEN_WORD);
        if(node) {
            //NOTE: Check if variable is already is in use
            char *name = nullTerminateArena(node->token.at, node->token.size, &globalPerFrameArena);
            AstVariable *variable = getCompilerVariable(state, name);
            if(!variable) {
                node = advanceAstNodeAndCheckType(state, TOKEN_EQUALS);
                if(node) {
                   //NOTE: Add variable as in use
                   pushCompilerVariable(state, name, AST_VARIABLE_NUMBER);
                   node = advanceAstNode(state);
                   state->currentNode = parseExpression(state, node);
                   if(!state->error) {
                        //NOTE: Now add the byte code for assigning variable
                        VmOperation data = { .type = OP_CODE_STRING, .name = name };
                        pushArrayItem(state->operations, data, VmOperation);

                        data = { .type = OP_CODE_VARIABLE_DECLARATION };
                        pushArrayItem(state->operations, data, VmOperation);
                   }
                }
            } else {
                state->error = easy_createString_printf(&globalPerFrameArena, "Variable name: %s already in use.", name);    
            }
        }
    }
}

void compileToByteCode(char *codeToCompile, VmOperation **operations) {
     EasyTokenizer tokenizer = lexBeginParsing(codeToCompile, EASY_LEX_OPTION_EAT_WHITE_SPACE);

    AstTree *tree = pushStruct(&globalPerFrameArena, AstTree);

    bool parsing = true;
    while(parsing) {
        EasyToken t = lexGetNextToken(&tokenizer);

        if(t.type == TOKEN_NULL_TERMINATOR) {
            parsing = false;
        } else if(t.type == TOKEN_SEMI_COLON) {
            //NOTE: Add string 

            VmOperation data = { .type = OP_CODE_PRINT };
            pushArrayItem(operations, data, VmOperation);
        } else if(t.type == TOKEN_OPEN_BRACKET) {
        } else if(t.type == TOKEN_INTEGER) {
            createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_NUMBER, .value_= (double)t.intVal }, getAstTypeForToken(t), getOperationType(t));
        } else if(t.type == TOKEN_FLOAT) { 
            createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_NUMBER, .value_= (double)t.floatVal }, getAstTypeForToken(t), getOperationType(t));
        } else if(t.type == TOKEN_PLUS) { 
            createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_ADD }, getAstTypeForToken(t), getOperationType(t));
        } else if(t.type == TOKEN_MINUS) { 
            createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_MINUS }, getAstTypeForToken(t), getOperationType(t));
        } else if(t.type == TOKEN_ASTRIX) { 
            createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_MULTIPLY }, getAstTypeForToken(t), getOperationType(t));
        } else if(t.type == TOKEN_FORWARD_SLASH) { 
            createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_DIVIDE }, getAstTypeForToken(t), getOperationType(t));
        } else if(t.type == TOKEN_NEWLINE) { 
            // VmOperation data = { .type = OP_CODE_PRINT };
            // pushArrayItem(operations, data, VmOperation);
        } else if(t.type == TOKEN_WORD) { 
            if(easyString_stringsMatch_null_and_count("sin", t.at, t.size)) {
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
            } else if(easyString_stringsMatch_null_and_count("let", t.at, t.size)) {
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_DECLARE }, getAstTypeForToken(t), getOperationType(t));
            } else {
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_VARIABLE_DECLARATION}, getAstTypeForToken(t, false), getOperationType(t));
            }
        }
    }

    CompilerState *state = pushStruct(&globalPerFrameArena, CompilerState);
    state->operations = operations;

    state->currentNode = tree->start;

    bool running = true;
    //NOTE: Now have ast, walk through this to output the vm instructions
    while(state->currentNode && running) {
        VmOperation op = state->currentNode->operation;
        if(op.type == OP_CODE_DECLARE) {
            //NOTE: Is 'let' symbol
            parseVariableDeclare(state);
        } else if(op.type == OP_CODE_VARIABLE_DECLARATION) {
            parseVariableAssign(state);
        } else {
            state->currentNode = parseExpression(state, state->currentNode);
        }
        if(state->error) {
            printf("%s\n", state->error);
            running = false;
        }
    }
}
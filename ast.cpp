
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

AstNode *createAndAddNode(AstTree *tree, AstPrecendence precedence, EasyToken token, VmOperation operation, AstType astType, AstOperationType operationType) {
    AstNode *node = pushStruct(&globalPerFrameArena, AstNode);
    node->type = astType;

    if(tree->current) {

        //NOTE: Popup back to last matching parent
        if(precedence == AST_POP) {
            //NOTE: Just pop once
            assert(tree->current->parent);
            tree->current = tree->current->parent;
        } else if(precedence == AST_POP_ALL) {
            //NOTE: Pop all the way back to the top of the tree.
            while(tree->current->parent) {
                assert(tree->current->parent);
                tree->current = tree->current->parent;
            }
        } else if(precedence != AST_SAME && precedence != AST_PUSH && (tree->current->precedence != AST_HIGHEST_PRECEDENCE || operationType == AST_OPERATION_POST)) {
            while(tree->current->parent && tree->current->precedence > precedence) {
                assert(tree->current->parent);
                tree->current = tree->current->parent;
            }
        }

        if(tree->current->precedence >= precedence || precedence == AST_POP) {
            //NOTE: Same precedence so keep on same level
            assert(!tree->current->next);
            tree->current->next = node;
            node->parent = tree->current->parent;

            if(precedence == AST_SAME || precedence == AST_POP) {
                precedence = tree->current->precedence;
            }
            if(tree->current->precedence == AST_HIGHEST_PRECEDENCE) {
                //NOTE: Default back to same value
                // tree->current->precedence = AST_SAME;
            }
        } else if(tree->current->child) {
            AstNode *parentNode = pushStruct(&globalPerFrameArena, AstNode);
            parentNode->type = AST_TYPE_PARENT;

            // parentNode->precedence = tree->current->precedence;
            parentNode->parent = tree->current->parent;
            tree->current->next = parentNode;
            tree->current = parentNode;
            node->parent = parentNode;

            if(node->type == AST_TYPE_VALUE) {
                precedence = AST_HIGHEST_PRECEDENCE;
            }


            tree->current->child = node;
        } else {
            assert(!tree->current->child);
            //NOTE: Make as child
            //NOTE: Make a parent node to set as the top, then bring down the current one with it
            if(tree->current->type == AST_TYPE_VALUE) {
                AstNode *childNode = pushStruct(&globalPerFrameArena, AstNode);
                *childNode = *tree->current;
                childNode->parent = tree->current;
                tree->current->type = AST_TYPE_PARENT;
                tree->current->child = childNode;
                node->parent = tree->current;
                childNode->next = node;
            } else {
                //NOTE: Just put the new node as a child
                node->parent = tree->current;
                tree->current->child = node;

                //NOTE: Put precedence as the highest level
                if(node->type != AST_TYPE_PARENT) {
                    // assert(node->type == AST_TYPE_VALUE);
                    precedence = AST_HIGHEST_PRECEDENCE;
                }

            }
        }
        tree->current = node;
    } else {
        assert(false);
    }

    node->precedence = precedence;
    node->token = token;
    node->operation = operation;
    node->operationType = operationType;


    return node;
}

bool compileToByteCode(char *codeToCompile, VmOperation **operations) {
     EasyTokenizer tokenizer = lexBeginParsing(codeToCompile, EASY_LEX_OPTION_EAT_WHITE_SPACE);

    AstTree *tree = pushStruct(&globalPerFrameArena, AstTree);

    CompilerState *state = pushStruct(&globalPerFrameArena, CompilerState);
    //NOTE: First node so just set it
    AstNode *startNode = pushStruct(&globalPerFrameArena, AstNode);
    startNode->type = AST_TYPE_PARENT;
    // startNode->operationType = AST_OPERATION_BEGIN_STATEMENT;
    tree->current = tree->start = startNode;

    bool parsing = true;
    while(parsing) {
        EasyToken t = lexGetNextToken(&tokenizer);

        if(t.type == TOKEN_NULL_TERMINATOR) {
            parsing = false;
        } else if(t.type == TOKEN_SEMI_COLON) {
            //NOTE: Don't add a semi colon, we're building an 'abstract' syntax tree, just end the statment
            astPopToRoot(tree);
            createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_NONE }, getAstTypeForToken(t), getOperationType(t));
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
            if(getOperationType(tree->lastToken) == AST_OPERATION_POST || tree->lastToken.type == TOKEN_UNINITIALISED || tree->lastToken.type == TOKEN_SEMI_COLON) {
                createAndAddNodeEmptyParent(tree);
                //NOTE: Is a unary negate operator
                createAndAddNode(tree, AST_HIGHEST_PRECEDENCE, t, { .type = OP_CODE_NEGATE}, getAstTypeForToken(t), getOperationType(t));
            } else {
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_MINUS }, getAstTypeForToken(t), getOperationType(t));
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
                createAndAddNodeEmptyParent(tree);
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_SIN }, getAstTypeForToken(t), getOperationType(t));
            } else if(easyString_stringsMatch_null_and_count("sqr", t.at, t.size)) {
                createAndAddNodeEmptyParent(tree);
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_SQR }, getAstTypeForToken(t), getOperationType(t));
            } else if(easyString_stringsMatch_null_and_count("sqrt", t.at, t.size)) {
                createAndAddNodeEmptyParent(tree);
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_SQRT }, getAstTypeForToken(t), getOperationType(t));
            } else if(easyString_stringsMatch_null_and_count("cos", t.at, t.size)) {
                createAndAddNodeEmptyParent(tree);
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_COS }, getAstTypeForToken(t), getOperationType(t));
            } else if(easyString_stringsMatch_null_and_count("tan", t.at, t.size)) {
                createAndAddNodeEmptyParent(tree);
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_TAN }, getAstTypeForToken(t), getOperationType(t));
            } else if(easyString_stringsMatch_null_and_count("asin", t.at, t.size)) {
                createAndAddNodeEmptyParent(tree);
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_ARCSIN }, getAstTypeForToken(t), getOperationType(t));
            } else if(easyString_stringsMatch_null_and_count("acos", t.at, t.size)) {
                createAndAddNodeEmptyParent(tree);
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_ARCCOS }, getAstTypeForToken(t), getOperationType(t));
            } else if(easyString_stringsMatch_null_and_count("atan", t.at, t.size)) {
                createAndAddNodeEmptyParent(tree);
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_ARCTAN }, getAstTypeForToken(t), getOperationType(t));
            } else {
                createAndAddNode(tree, getPrecedenceForToken(t), t, { .type = OP_CODE_VARIABLE_REFERENCE}, getAstTypeForToken(t, false), getOperationType(t));
            }
        }
        tree->lastToken = t;
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

            if(state->error) {
                printf("%s\n", state->error);
                running = false;
            }
        }
    }

    return state->error != 0;
}
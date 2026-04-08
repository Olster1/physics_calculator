
enum AstVariableType {
    AST_VARIABLE_NUMBER,
    AST_VARIABLE_STRING,
    AST_VARIABLE_BOOLEAN,
};

struct AstVariable {
    AstVariableType type;
    char *name;

    AstVariable *next;
};

struct CompilerState {
    char *error;
    AstVariable *variables[MAX_VARIABLE_MAP_SIZE]; //NOTE: Nodes pushed onto per frame arena

    Map<char *, VmOperation> functionCalls;

    int calculatorLineAt;

    List<VmOperation> *operations; //NOTE: Resize array
};

void initCompiler(CompilerState *state) {
    state->functionCalls = Map<char *, VmOperation>::init(&globalPerFrameArena);
    state->functionCalls.insert("sin", { .type = OP_CODE_SIN });
    state->functionCalls.insert("cos", { .type = OP_CODE_COS });
    state->functionCalls.insert("tan", { .type = OP_CODE_TAN });
    state->functionCalls.insert("asin", { .type = OP_CODE_ARCSIN });
    state->functionCalls.insert("acos", { .type = OP_CODE_ARCCOS });
    state->functionCalls.insert("atan", { .type = OP_CODE_ARCTAN });
    state->functionCalls.insert("atan2", { .type = OP_CODE_ARCTAN2 });
    state->functionCalls.insert("sqr", { .type = OP_CODE_SQR });
    state->functionCalls.insert("sqrt", { .type = OP_CODE_SQRT });

    assert(state->functionCalls.get("sin"));
}

void pushCompilerVariable(CompilerState *state, char *name, AstVariableType type) {
    AstVariable *var = pushStruct(&globalPerFrameArena, AstVariable);

    var->name = name;
    var->type = type;

    int index = getIndexForVariableMap(name);

    AstVariable **ptr = &state->variables[index];
    while(*ptr) {
        ptr = &(*ptr)->next;
    }
    *ptr = var;
}

AstVariable *getCompilerVariable(CompilerState *state, char *name) {
    int index = getIndexForVariableMap(name);
    AstVariable *ptr = state->variables[index];

    bool found = false;
    while(ptr && !found) {
        if(easyString_stringsMatch_nullTerminated(name, ptr->name)) {

            found = true;
            break;
        } else {
            ptr = ptr->next;
        }
    }
    return ptr;
}

void pushPrintOperation(CompilerState *state) {
    VmOperation data = { .type = OP_CODE_NUMBER, .value_ = (double)(state->calculatorLineAt++) };
    state->operations->push(data);

    data = { .type = OP_CODE_PRINT };
    state->operations->push(data);
}

AstExpression *interpretExpression(CompilerState *state, AstExpression *expression);

void interpretLiteralExpression(CompilerState *state, AstExpression *expression) {
    if(expression->token.type == TOKEN_INTEGER) {
        VmOperation op = { .type = OP_CODE_NUMBER, .value_ = (double)expression->token.intVal };
        state->operations->push(op);
    } else if(expression->token.type == TOKEN_FLOAT) {
        VmOperation op = { .type = OP_CODE_NUMBER, .value_ = expression->token.floatVal };
        state->operations->push(op);
    } else {
        assert(false);
    }
}

void interpretAssignExpression(CompilerState *state, AstExpression *expression) {
    assert(expression->right);
    interpretExpression(state, expression->right);

    //NOTE: assigns have to be just one word
    assert(expression->left);
    assert(expression->left->token.type == TOKEN_WORD);
    assert(expression->left->type == AST_EXPRESSION_TYPE_NAMED);

    char *name = nullTerminateArena(expression->left->token.at, expression->left->token.size, &globalPerFrameArena);

    AstVariable *variable = getCompilerVariable(state, name);

    if(!variable) {
        printf("Variable not declared. Adding it now.\n");
        pushCompilerVariable(state, name, AST_VARIABLE_NUMBER);
    }

    VmOperation op = { .type = OP_CODE_VARIABLE_ASSIGN, .name = name };
    state->operations->push(op);
}

void interpretBlockExpression(CompilerState *state, AstExpression *expression) {
    for(int i = 0; i < expression->arguments.count; ++i) {
        interpretExpression(state, expression->arguments[i]);
        //TODO: This is specific to our calculator program. In a real programming language this wouldn't be here.
        pushPrintOperation(state);
    }
}

void interpretCallExpression(CompilerState *state, AstExpression *expression) {
    for(int i = 0; i < expression->arguments.count; ++i) {
        interpretExpression(state, expression->arguments[i]);
    }

    //NOTE: Functions have to be just one word
    assert(expression->left);
    assert(expression->left->token.type == TOKEN_WORD);
    assert(expression->left->type == AST_EXPRESSION_TYPE_NAMED);

    char *name = nullTerminateArena(expression->left->token.at, expression->left->token.size, &globalPerFrameArena);

    VmOperation *op = state->functionCalls.get(name);
    if(!op) {
        state->error = "Function not declared";
    } else {
        state->operations->push(*op);
    }
}

void interpretPostfixExpression(CompilerState *state, AstExpression *expression) {
    switch(expression->token.type) {
        default: {
            assert(false);
        } break;
    }
}

void interpretPrefixExpression(CompilerState *state, AstExpression *expression) {
    assert(expression->right);
    interpretExpression(state, expression->right);

    switch(expression->token.type) {
        case TOKEN_MINUS: {
            //NOTE: Negate operation
            VmOperation op = { .type = OP_CODE_NEGATE };
            state->operations->push(op);
        } break;
        default: {
            assert(false);
        } break;
    }
}


void interpretOperatorExpression(CompilerState *state, AstExpression *expression) {
    assert(expression->left);
    assert(expression->right);
    interpretExpression(state, expression->left);
    interpretExpression(state, expression->right);

    switch(expression->token.type) {
        case TOKEN_PLUS: {
            VmOperation op = { .type = OP_CODE_ADD };
            state->operations->push(op);
        } break;
        case TOKEN_MINUS: {
            VmOperation op = { .type = OP_CODE_MINUS };
            state->operations->push(op);
        } break;
        case TOKEN_ASTRIX: {
            VmOperation op = { .type = OP_CODE_MULTIPLY };
            state->operations->push(op);
        } break;
        case TOKEN_FORWARD_SLASH: {
            VmOperation op = { .type = OP_CODE_DIVIDE };
            state->operations->push(op);
        } break;
        case TOKEN_CARROT: {
            VmOperation op = { .type = OP_CODE_POWER_TO };
            state->operations->push(op);
        } break;
        default: {
            assert(false);
        } break;
    }

}

AstExpression *interpretExpression(CompilerState *state, AstExpression *expression) {
    switch(expression->type) {
        case AST_EXPRESSION_TYPE_BLOCK: {
            interpretBlockExpression(state, expression);
        } break;
        case AST_EXPRESSION_TYPE_ASSIGN: {
            interpretAssignExpression(state, expression);
        } break;
        case AST_EXPRESSION_TYPE_LITERAL: {
            interpretLiteralExpression(state, expression);
        } break;
        case AST_EXPRESSION_TYPE_NAMED: {
            char *name = nullTerminateArena(expression->token.at, expression->token.size, &globalPerFrameArena);
            AstVariable *variable = getCompilerVariable(state, name);

            if(!variable) {
                state->error = "Variable not declared";
            } else {
                VmOperation op = { .type = OP_CODE_VARIABLE_REFERENCE, .name = name };
                state->operations->push(op);
            }
        } break;
        case AST_EXPRESSION_TYPE_OPERATOR: {
            interpretOperatorExpression(state, expression);
        } break;
        case AST_EXPRESSION_TYPE_PREFIX: {
            interpretPrefixExpression(state, expression);
        } break;
        case AST_EXPRESSION_TYPE_POSTFIX: {
             interpretPostfixExpression(state, expression);
        } break;
        case AST_EXPRESSION_TYPE_CALL: {
            interpretCallExpression(state, expression);
        } break;
        default: {
            assert(false);
        } break;
    }
    return 0;
}
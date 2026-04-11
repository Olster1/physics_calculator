
enum AstVariableType {
    AST_VARIABLE_NUMBER,
    AST_VARIABLE_STRING,
    AST_VARIABLE_BOOLEAN,
    AST_VARIABLE_ARRAY,
};

struct AstVariable {
    AstVariableType type;
    char *name;

    AstVariable *next;
};

enum InterpreterFunctionFlags {
    INTERP_FUNCTION_VARIABLE_LENGTH = 1 << 0,
};

struct InterpreterFunction{
    VmOperation operation;

    u64 flags;
    List<AstVariableType> arguments;
    List<AstVariableType> returnArguments;

    static InterpreterFunction init(VmOperation operation, u64 flags = 0) {
        InterpreterFunction result = {};

        result.flags = flags;
        result.operation = operation;
        result.arguments = List<AstVariableType>::init(&globalPerFrameArena);
        result.returnArguments = List<AstVariableType>::init(&globalPerFrameArena);

        return result;
    }

    void pushArgument(AstVariableType type) {
        arguments.push(type);
    }
    void pushReturnType(AstVariableType type) {
        returnArguments.push(type);
    }
};

struct CompilerState {
    ExpressionParser parser;

    AstVariable *variables[MAX_VARIABLE_MAP_SIZE]; //NOTE: Nodes pushed onto per frame arena

    Map<char *, InterpreterFunction> functionCalls;

    int calculatorLineAt;

    List<VmOperation> *operations; //NOTE: Resize array
};

void initCompiler(CompilerState *state) {
    state->parser.error = 0;

    state->functionCalls = Map<char *, InterpreterFunction>::init(&globalPerFrameArena);

    state->functionCalls.insert("quad", InterpreterFunction::init({ .type = OP_CODE_QUADRATIC }));
    state->functionCalls.insert("sin", InterpreterFunction::init({ .type = OP_CODE_SIN }));
    state->functionCalls.insert("cos", InterpreterFunction::init({ .type = OP_CODE_COS }));
    state->functionCalls.insert("tan", InterpreterFunction::init({ .type = OP_CODE_TAN }));
    state->functionCalls.insert("asin", InterpreterFunction::init({ .type = OP_CODE_ARCSIN }));
    state->functionCalls.insert("acos", InterpreterFunction::init({ .type = OP_CODE_ARCCOS }));
    state->functionCalls.insert("atan", InterpreterFunction::init({ .type = OP_CODE_ARCTAN }));
    state->functionCalls.insert("atan2", InterpreterFunction::init({ .type = OP_CODE_ARCTAN2 }));
    state->functionCalls.insert("sqr", InterpreterFunction::init({ .type = OP_CODE_SQR }));
    state->functionCalls.insert("sqrt", InterpreterFunction::init({ .type = OP_CODE_SQRT }));

    state->functionCalls.insert("dot", InterpreterFunction::init({ .type = OP_CODE_DOT_PRODUCT }));
    state->functionCalls.insert("cross", InterpreterFunction::init({ .type = OP_CODE_CROSS_PRODUCT }));
    state->functionCalls.insert("length", InterpreterFunction::init({ .type = OP_CODE_VECTOR_LENGTH }));

    state->functionCalls.insert("sum", InterpreterFunction::init({ .type = OP_CODE_SUMMATION }, INTERP_FUNCTION_VARIABLE_LENGTH));

    state->functionCalls.insert("set_radians", InterpreterFunction::init({ .type = OP_CODE_SET_RADIANS_MODE }));
    state->functionCalls.insert("set_degrees", InterpreterFunction::init({ .type = OP_CODE_SET_DEGREES_MODE }));
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

void interpretExpression(CompilerState *state, AstExpression *expression);

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
        assert(getCompilerVariable(state, name));
    }

    VmOperation op = { .type = OP_CODE_VARIABLE_ASSIGN, .name = name };
    state->operations->push(op);

    //NOTE: For our calculator we now want to push the variable back on the VM stack so the print operation can grab it
    state->operations->push({ .type = OP_CODE_VARIABLE_REFERENCE, .name = name });
}

void interpretBlockExpression(CompilerState *state, AstExpression *expression) {
    for(int i = 0; i < expression->arguments.count; ++i) {
        interpretExpression(state, expression->arguments[i]);
        //TODO: This is specific to our calculator program. In a real programming language this wouldn't be here.
        pushPrintOperation(state);
    }
}

void interpretArrayExpression(CompilerState *state, AstExpression *expression) {
    for(int i = expression->arguments.count - 1; i >= 0 ; --i) {
        interpretExpression(state, expression->arguments[i]);
    }
    state->operations->push({ .type = OP_CODE_NUMBER_ARRAY, .value_ = (double)expression->arguments.count });

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

    InterpreterFunction *op = state->functionCalls.get(name);
    if(!op) {
        state->parser.error = "Function not declared";
    } else {
        state->operations->push(op->operation);
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
    if(expression->right) {
        interpretExpression(state, expression->right);
    }

    switch(expression->token.type) {
        case TOKEN_MINUS: {
            //NOTE: Negate operation
            VmOperation op = { .type = OP_CODE_NEGATE };
            state->operations->push(op);
        } break;
        case TOKEN_OPEN_SQUARE_BRACKET: {
            //NOTE: Array declaration
            interpretArrayExpression(state, expression);
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

void interpretExpression(CompilerState *state, AstExpression *expression) {
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
                state->parser.error = "Variable not declared";
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
}
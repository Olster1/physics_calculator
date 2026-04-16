#define AST_TYPE_CHECKTYPE(FUNC) \
FUNC(AST_VARIABLE_NONE) \
FUNC(AST_VARIABLE_NUMBER) \
FUNC(AST_VARIABLE_U64) \
FUNC(AST_VARIABLE_STRING) \
FUNC(AST_VARIABLE_BOOLEAN) \

typedef enum {
    AST_TYPE_CHECKTYPE(ENUM)
} AstVariableType;

static char *AstVariableTypeStrings[] = { "none", "number", "string", "boolean" };


struct TypeCheckerType {
    AstVariableType type;
    int count; //NOTE: If more than 1 it's an array type
    bool isArray; //NOTE: To differentiate between an array of size 1 and plain 1 variable
};

struct AstVariable {
    TypeCheckerType type;
    char *name;

    AstVariable *next;
};

enum InterpreterFunctionFlags {
    INTERP_FUNCTION_VARIABLE_LENGTH = 1 << 0,
};

struct InterpreterFunction{
    VmOperation operation;

    u64 flags;
    List<TypeCheckerType> arguments;
    TypeCheckerType returnArgument;

    static InterpreterFunction init(VmOperation operation, u64 flags = 0) {
        InterpreterFunction result = {};

        result.flags = flags;
        result.operation = operation;
        result.arguments = List<TypeCheckerType>::init(&globalPerFrameArena);

        return result;
    }

    void pushArgument(TypeCheckerType type) {
        arguments.push(type);
    }
    void setReturnType(TypeCheckerType type) {
        returnArgument = type;
    }

    //NOTE: Functions can be overloaded i.e. different arguments or return types. If something is overloaded we store the next function on the pointer
    //      This probably would be slow if the user had lots and lots of overloaded functions but most functions aren't overloaded
    InterpreterFunction *overloadFunctions;
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

    {
        InterpreterFunction func = InterpreterFunction::init({ .type = OP_CODE_QUADRATIC });
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 1});
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 1});
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 1});
        func.setReturnType({.type = AST_VARIABLE_NUMBER, .count = 2, .isArray = true});
        state->functionCalls.insert("quad", func);
    }

    {
        InterpreterFunction func = InterpreterFunction::init({ .type = OP_CODE_SIN });
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 1});
        func.setReturnType({.type = AST_VARIABLE_NUMBER, .count = 1});
        state->functionCalls.insert("sin", func);
    }
    {
        InterpreterFunction func = InterpreterFunction::init({ .type = OP_CODE_COS });
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 1});
        func.setReturnType({.type = AST_VARIABLE_NUMBER, .count = 1});
        state->functionCalls.insert("cos", func);
    }
    {
        InterpreterFunction func = InterpreterFunction::init({ .type = OP_CODE_TAN });
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 1});
        func.setReturnType({.type = AST_VARIABLE_NUMBER, .count = 1});
        state->functionCalls.insert("tan", func);
    }
    {
        InterpreterFunction func = InterpreterFunction::init({ .type = OP_CODE_ARCSIN });
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 1});
        func.setReturnType({.type = AST_VARIABLE_NUMBER, .count = 1});
        state->functionCalls.insert("asin", func);
    }
    {
        InterpreterFunction func = InterpreterFunction::init({ .type = OP_CODE_ARCCOS });
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 1});
        func.setReturnType({.type = AST_VARIABLE_NUMBER, .count = 1});
        state->functionCalls.insert("acos", func);
    }
    {
        InterpreterFunction func = InterpreterFunction::init({ .type = OP_CODE_ARCTAN });
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 1});
        func.setReturnType({.type = AST_VARIABLE_NUMBER, .count = 1});
        state->functionCalls.insert("atan", func);
    }
    {
        InterpreterFunction func = InterpreterFunction::init({ .type = OP_CODE_ARCTAN2 });
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 1});
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 1});
        func.setReturnType({.type = AST_VARIABLE_NUMBER, .count = 1});
        state->functionCalls.insert("atan2", func);
    }
    {
        InterpreterFunction func = InterpreterFunction::init({ .type = OP_CODE_SQR });
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 1});
        func.setReturnType({.type = AST_VARIABLE_NUMBER, .count = 1});
        state->functionCalls.insert("sqr", func);
    }
    {
        InterpreterFunction func = InterpreterFunction::init({ .type = OP_CODE_SQRT });
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 1});
        func.setReturnType({.type = AST_VARIABLE_NUMBER, .count = 1});
        state->functionCalls.insert("sqrt", func);
    }
    {
        InterpreterFunction func = InterpreterFunction::init({ .type = OP_CODE_DOT_PRODUCT });
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 2});
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 2});
        func.setReturnType({.type = AST_VARIABLE_NUMBER, .count = 1});

        //NOTE: Overloaded
        InterpreterFunction *func2 = pushStruct(&globalPerFrameArena, InterpreterFunction);
        *func2 = InterpreterFunction::init({ .type = OP_CODE_DOT_PRODUCT });
        func2->pushArgument({.type = AST_VARIABLE_NUMBER, .count = 3});
        func2->pushArgument({.type = AST_VARIABLE_NUMBER, .count = 3});
        func2->setReturnType({.type = AST_VARIABLE_NUMBER, .count = 1});
        func.overloadFunctions = func2;

        state->functionCalls.insert("dot", func);
    }
    {
        InterpreterFunction func = InterpreterFunction::init({ .type = OP_CODE_CROSS_PRODUCT });
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 2});
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 2});
        func.setReturnType({.type = AST_VARIABLE_NUMBER, .count = 2, .isArray = true});

        //NOTE: Overloaded
        InterpreterFunction *func2 = pushStruct(&globalPerFrameArena, InterpreterFunction);
        *func2 = InterpreterFunction::init({ .type = OP_CODE_CROSS_PRODUCT });
        func2->pushArgument({.type = AST_VARIABLE_NUMBER, .count = 3});
        func2->pushArgument({.type = AST_VARIABLE_NUMBER, .count = 3});
        func2->setReturnType({.type = AST_VARIABLE_NUMBER, .count = 3, .isArray = true});
        func.overloadFunctions = func2;

        state->functionCalls.insert("cross", func);
    }

    {
        InterpreterFunction func = InterpreterFunction::init({ .type = OP_CODE_VECTOR_LENGTH });
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = 2});
        func.setReturnType({.type = AST_VARIABLE_NUMBER, .count = 1});

        //NOTE: Overloaded
        InterpreterFunction *func2 = pushStruct(&globalPerFrameArena, InterpreterFunction);
        *func2 = InterpreterFunction::init({ .type = OP_CODE_VECTOR_LENGTH });
        func2->pushArgument({.type = AST_VARIABLE_NUMBER, .count = 3});
        func2->setReturnType({.type = AST_VARIABLE_NUMBER, .count = 1});
        func.overloadFunctions = func2;

        state->functionCalls.insert("length", func);
    }

    {
        InterpreterFunction func = InterpreterFunction::init({ .type = OP_CODE_SUMMATION });
        func.pushArgument({.type = AST_VARIABLE_NUMBER, .count = -1});
        func.setReturnType({.type = AST_VARIABLE_NUMBER, .count = 1});
        state->functionCalls.insert("sum", func);
    }

    state->functionCalls.insert("set_radians", InterpreterFunction::init({ .type = OP_CODE_SET_RADIANS_MODE }));
    state->functionCalls.insert("set_degrees", InterpreterFunction::init({ .type = OP_CODE_SET_DEGREES_MODE }));
}

void pushCompilerVariable(CompilerState *state, char *name, TypeCheckerType type) {
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

TypeCheckerType typeCheckExpression(CompilerState *state, AstExpression *expression);

TypeCheckerType typeCheckLiteralExpression(CompilerState *state, AstExpression *expression) {
    TypeCheckerType type = {};
    if(expression->token.type == TOKEN_INTEGER) {
        type = { .type = AST_VARIABLE_NUMBER, .count = 1 };
    } else if(expression->token.type == TOKEN_FLOAT) {
        type = { .type = AST_VARIABLE_NUMBER, .count = 1 };
    } else {
        assert(false);
    }
    return type;
}

TypeCheckerType typeCheckAssignExpression(CompilerState *state, AstExpression *expression) {
    assert(expression->right);
    TypeCheckerType rightType =  typeCheckExpression(state, expression->right);

    //NOTE: assigns have to be just one word
    assert(expression->left);
    assert(expression->left->token.type == TOKEN_WORD);
    assert(expression->left->type == AST_EXPRESSION_TYPE_NAMED);

    char *name = nullTerminateArena(expression->left->token.at, expression->left->token.size, &globalPerFrameArena);

    AstVariable *variable = getCompilerVariable(state, name);

    if(!variable) {
        printf("Variable not declared. Adding it now.\n");
        pushCompilerVariable(state, name, rightType);
        assert(getCompilerVariable(state, name));
    } else if(variable->type.type != rightType.type) {
        //NOTE: Check if the user is changing it's type
        variable->type = rightType;
    } else if(variable->type.count != rightType.count) {
        //NOTE: Check if the user is changing it's count
        variable->type = rightType;
    }

    return rightType;
}

void typeCheckBlockExpression(CompilerState *state, AstExpression *expression) {
    for(int i = 0; i < expression->arguments.count; ++i) {
        typeCheckExpression(state, expression->arguments[i]);
    }
}

TypeCheckerType typeCheckArrayExpression(CompilerState *state, AstExpression *expression) {
    TypeCheckerType result ={ .type = AST_VARIABLE_NUMBER, .count = expression->arguments.count, .isArray = true };
    for(int i = expression->arguments.count - 1; i >= 0 ; --i) {
        TypeCheckerType t = typeCheckExpression(state, expression->arguments[i]);
        if(t.type != AST_VARIABLE_NUMBER || t.count != 1) {
            state->parser.logError("Arguments of an array must be a number and not an array");
        }
    }
    return result;
}

TypeCheckerType typeCheckCallExpression(CompilerState *state, AstExpression *expression) {
    TypeCheckerType result = {};
    //NOTE: Functions have to be just one word
    assert(expression->left);
    assert(expression->left->token.type == TOKEN_WORD);
    assert(expression->left->type == AST_EXPRESSION_TYPE_NAMED);

    char *name = nullTerminateArena(expression->left->token.at, expression->left->token.size, &globalPerFrameArena);

    InterpreterFunction *functionType = state->functionCalls.get(name);
    if(!functionType) {
        state->parser.error = "Function not declared";
    } else {
        bool foundMatch = false;
        char *errorToAssign = 0;
        while(functionType && !foundMatch) {
            result = functionType->returnArgument;
            bool functionMatches = true;

            if(functionType->arguments.count != expression->arguments.count) {
                char *extraS = "";
                if(functionType->arguments.count != 1) {
                    extraS = "s";
                }
                errorToAssign = easy_createString_printf(&globalPerFrameArena, "Expected %d argument%s, got %d.", functionType->arguments.count, extraS, expression->arguments.count);
                functionMatches = false;
            } else {
                for(int i = 0; i < expression->arguments.count; ++i) {
                    TypeCheckerType type = functionType->arguments[i];
                    TypeCheckerType t = typeCheckExpression(state, expression->arguments[i]);

                    bool countWrong = type.count != t.count;

                    if(type.count < 0) {
                        //NOTE: Expects an array as input so check that it is an array and is size bigger than zero
                        if(t.count == 0 || !t.isArray) {
                            errorToAssign = "Function expects an array";
                            functionMatches = false;
                        }
                        //NOTE: The function input type is variable length and there is actually at least one number in the return type array
                        countWrong = false;
                    }

                    if(type.type != t.type || countWrong) {
                        errorToAssign = easy_createString_printf(&globalPerFrameArena, "Expected type: '%s' of size %d, got type: '%s' of size %d at position %d", AstVariableTypeStrings[type.type], type.count,  AstVariableTypeStrings[t.type], t.count, i + 1);
                        functionMatches = false;
                    }
                }

            }
            if(functionMatches) {
                foundMatch = true;
            }

            functionType = functionType->overloadFunctions;
        }
        if(errorToAssign && !foundMatch) {
            state->parser.logError(errorToAssign);
        }
    }
    return result;
}

TypeCheckerType typeCheckPostfixExpression(CompilerState *state, AstExpression *expression) {
    TypeCheckerType result = {};
    switch(expression->token.type) {
        default: {
            assert(false);
        } break;
    }
    return result;
}

TypeCheckerType typeCheckPrefixExpression(CompilerState *state, AstExpression *expression) {
    TypeCheckerType result = {};
    if(expression->right) {
        result = typeCheckExpression(state, expression->right);
    }

    switch(expression->token.type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS: {
            //NOTE: Negate operation
            if(result.type != AST_VARIABLE_NUMBER) {
                state->parser.logError("Can only negate, number types");
            }
            if(result.isArray) {
                state->parser.logError("You can't negate arrays");
            }
        } break;
        case TOKEN_U64_TYPE: {
            if(result.type != AST_VARIABLE_NUMBER) {
                state->parser.logError("Can only type cast number types");
            }
            if(result.isArray) {
                state->parser.logError("You can't typecast arrays");
            }
        } break;
        case TOKEN_OPEN_SQUARE_BRACKET: {
            //NOTE: Array declaration
            result = typeCheckArrayExpression(state, expression);
        } break;
        default: {
            // state->parser.logError("Can only negate number types");
            assert(false);
        } break;
    }
    return result;
}


TypeCheckerType typeCheckOperatorExpression(CompilerState *state, AstExpression *expression) {
    TypeCheckerType type = {};

    assert(expression->left);
    assert(expression->right);
    TypeCheckerType leftType = typeCheckExpression(state, expression->left);
    TypeCheckerType rightType = typeCheckExpression(state, expression->right);

    if(leftType.type != AST_VARIABLE_NUMBER) {
        state->parser.logError("Left Hand operand must be a number");
    } else if(rightType.type != AST_VARIABLE_NUMBER) {
        state->parser.logError("Right Hand operand must be a number");
    } else if(leftType.isArray) {
        state->parser.logError("Left Hand operand cant be an array");
    } else if(rightType.isArray) {
        state->parser.logError("Right Hand operand cant be an array");
    } else {
        switch(expression->token.type) {
            case TOKEN_PLUS:
            case TOKEN_MINUS:
            case TOKEN_ASTRIX:
            case TOKEN_FORWARD_SLASH:
            case TOKEN_BIT_SHIFT_RIGHT:
            case TOKEN_BIT_SHIFT_LEFT:
            case TOKEN_BIT_AND:
            case TOKEN_BIT_OR:
            case TOKEN_CARROT: {
                type = { .type = AST_VARIABLE_NUMBER, .count = 1 };
            } break;
            default: {
                assert(false);
            } break;
        }
    }


    return type;

}

TypeCheckerType typeCheckExpression(CompilerState *state, AstExpression *expression) {
    TypeCheckerType result = {};
    switch(expression->type) {
        case AST_EXPRESSION_TYPE_BLOCK: {
            typeCheckBlockExpression(state, expression);
        } break;
        case AST_EXPRESSION_TYPE_ASSIGN: {
            result = typeCheckAssignExpression(state, expression);
        } break;
        case AST_EXPRESSION_TYPE_LITERAL: {
            result = typeCheckLiteralExpression(state, expression);
        } break;
        case AST_EXPRESSION_TYPE_NAMED: {
            char *name = nullTerminateArena(expression->token.at, expression->token.size, &globalPerFrameArena);
            AstVariable *variable = getCompilerVariable(state, name);

            if(!variable) {
                state->parser.error = "Variable not declared";
            } else {
                result = variable->type;
            }
        } break;
        case AST_EXPRESSION_TYPE_OPERATOR: {
            result = typeCheckOperatorExpression(state, expression);
        } break;
        case AST_EXPRESSION_TYPE_PREFIX: {
            result = typeCheckPrefixExpression(state, expression);
        } break;
        case AST_EXPRESSION_TYPE_POSTFIX: {
             result = typeCheckPostfixExpression(state, expression);
        } break;
        case AST_EXPRESSION_TYPE_CALL: {
            result = typeCheckCallExpression(state, expression);
        } break;
        default: {
            assert(false);
        } break;
    }
    return result;
}
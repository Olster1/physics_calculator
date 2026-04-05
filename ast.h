
enum AstPrecendence {
    AST_NONE,
    AST_PUSH,
    AST_POP_ALL,
    AST_POP,
    AST_SAME,
    AST_PLUS_MINUS,
    AST_MULTIPLY_DIVIDE,
    AST_HIGHEST_PRECEDENCE_SAME,
    AST_HIGHEST_PRECEDENCE,
    AST_EXPONENT,
};

enum AstType {
    AST_TYPE_DEFAULT,
    AST_TYPE_VALUE,
    AST_TYPE_OPERATION,
    AST_TYPE_FUNC,
    AST_TYPE_PARENT,
};

enum AstOperationType {
    AST_OPERATION_NONE,
    AST_OPERATION_BEGIN_STATEMENT,
    AST_OPERATION_PRE,
    AST_OPERATION_POST,
};

AstOperationType getOperationType(EasyToken t) {
    AstOperationType result = AST_OPERATION_PRE;
    switch(t.type) {
        case TOKEN_ASTRIX:
        case TOKEN_FORWARD_SLASH:
        case TOKEN_PLUS:
        case TOKEN_CARROT:
        case TOKEN_MINUS: {
            result = AST_OPERATION_POST;
        } break;
        case TOKEN_SEMI_COLON: {
            result = AST_OPERATION_BEGIN_STATEMENT;
        }
        default: {

        }
    }
    return result;
}

AstType getAstTypeForToken(EasyToken t, bool isKeyword = true) {
    AstType result = AST_TYPE_DEFAULT;
    switch(t.type) {
        case TOKEN_WORD: {
            if(isKeyword) {
                result = AST_TYPE_FUNC;
            } else {
                result = AST_TYPE_VALUE;
            }
        } break;
        case TOKEN_OPEN_PARENTHESIS: {
            result = AST_TYPE_PARENT;
        } break;
        case TOKEN_EQUALS:
        case TOKEN_PLUS:
        case TOKEN_CLOSE_PARENTHESIS:
        case TOKEN_ASTRIX:
        case TOKEN_CARROT:
        case TOKEN_FORWARD_SLASH:
        case TOKEN_MINUS: {
            result = AST_TYPE_OPERATION;
        } break;
        case TOKEN_FLOAT:
        case TOKEN_INTEGER: {
            result = AST_TYPE_VALUE;
        } break;
        default: {

        }
    }
    return result;
}

struct AstNode {
    AstType type;
    //NOTE: These are the token data
    AstPrecendence precedence;
    EasyToken token;
    VmOperation operation; //NOTE: easy to just add it when we run the lexer

    AstOperationType operationType;

    //NOTE: These make up the graph connections
    AstNode *next;
    AstNode *child;
    AstNode *parent;
};

struct AstTree {
    AstNode *start;
    AstNode *current;
    EasyToken lastToken;
};


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


void astPopToRoot(AstTree *tree) {
    if(tree->current) {
        //NOTE: Pop all the way back to the top of the tree.
        while(tree->current->parent) {
            assert(tree->current->parent);
            tree->current = tree->current->parent;
        }
    }
}

AstNode *createAndAddNodeEmptyParent(AstTree *tree) {
    AstNode *parentNode = pushStruct(&globalPerFrameArena, AstNode);
    parentNode->type = AST_TYPE_PARENT;

    parentNode->parent = tree->current->parent;
    tree->current->next = parentNode;
    tree->current = parentNode;
    return parentNode;
}

AstPrecendence getPrecedenceForToken(EasyToken t) {
    AstPrecendence precedence = AST_NONE;
    switch(t.type) {
        case TOKEN_SEMI_COLON: {
            precedence = AST_POP_ALL;
        } break;
        case TOKEN_PLUS:
        case TOKEN_MINUS: {
            precedence = AST_PLUS_MINUS;
        } break;
        case TOKEN_EQUALS:
        case TOKEN_WORD:
        case TOKEN_FLOAT:
        case TOKEN_INTEGER: {
            precedence = AST_SAME;
        } break;
        case TOKEN_ASTRIX:
        case TOKEN_FORWARD_SLASH: {
            precedence = AST_MULTIPLY_DIVIDE;
        } break;
        case TOKEN_CARROT: {
            precedence = AST_EXPONENT;
        } break;
        case TOKEN_OPEN_PARENTHESIS: {
            precedence = AST_HIGHEST_PRECEDENCE;
        } break;
        case TOKEN_CLOSE_PARENTHESIS: {
            precedence = AST_POP;
        } break;
        default: {

        }
    }
    return precedence;
}

struct CompilerState {
    char *error;
    AstNode *currentNode;
    AstVariable *variables[MAX_VARIABLE_MAP_SIZE]; //NOTE: Nodes pushed onto per frame arena

    int calculatorLineAt;

    VmOperation **operations; //NOTE: Resize array
};

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
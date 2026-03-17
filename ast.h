enum AstPrecendence {
    AST_NONE,
    AST_POP,
    AST_SAME,
    AST_PLUS_MINUS,
    AST_MULTIPLY_DIVIDE,
    AST_PUSH,
};

enum AstType {
    AST_TYPE_DEFAULT,
    AST_TYPE_VALUE,
    AST_TYPE_OPERATION,
    AST_TYPE_PARENT,
};

enum AstOperationType {
    AST_OPERATION_NONE,
    AST_OPERATION_PRE,
    AST_OPERATION_POST,
};

AstOperationType getOperationType(EasyToken t) {
    AstOperationType result = AST_OPERATION_PRE;
    switch(t.type) {
        case TOKEN_ASTRIX:
        case TOKEN_FORWARD_SLASH:
        case TOKEN_PLUS:
        case TOKEN_MINUS: {
            result = AST_OPERATION_POST;
        } break;
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
                result = AST_TYPE_OPERATION;
            } else {
                result = AST_TYPE_VALUE;
            }
        } break;
        case TOKEN_EQUALS:
        case TOKEN_PLUS:
        case TOKEN_OPEN_PARENTHESIS:
        case TOKEN_CLOSE_PARENTHESIS:
        case TOKEN_ASTRIX:
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
};

AstNode *createAndAddNode(AstTree *tree, AstPrecendence precedence, EasyToken token, VmOperation operation, AstType astType, AstOperationType operationType) {
    AstNode *node = pushStruct(&globalPerFrameArena, AstNode);
    node->type = astType;

    if(tree->current) {
        //NOTE: Popup back to last matching parent
        if(precedence == AST_POP) {
            //NOTE: Just pop once
            assert(tree->current->parent);
            tree->current = tree->current->parent;
        } else if(precedence != AST_SAME) {
            while(tree->current->parent && tree->current->precedence > node->precedence) {
                assert(tree->current->parent);
                tree->current = tree->current->parent;
                //NOTE: Check if this is just one pop
            }
        }

        if(tree->current->precedence >= node->precedence || precedence == AST_SAME || precedence == AST_POP) {
            //NOTE: Same precedence so keep on same level
            assert(!tree->current->next);
            tree->current->next = node;
            node->parent = tree->current->parent;

            if(precedence == AST_SAME || precedence == AST_POP) {
                precedence = tree->current->precedence;
            }
        } else {
            assert(!tree->current->child);
            //NOTE: Make as child
            //NOTE: Make a parent node to set as the top, then bring down the current one with it
            AstNode *childNode = pushStruct(&globalPerFrameArena, AstNode);
            *childNode = *tree->current;
            childNode->parent = tree->current;
            tree->current->type = AST_TYPE_PARENT;
            tree->current->child = childNode;
            node->parent = tree->current;
            childNode->next = node;
        }
        tree->current = node;
    } else {
        //NOTE: First node so just set it
        tree->current = tree->start = node;
    }

    node->precedence = precedence;
    node->token = token;
    node->operation = operation;
    node->operationType = operationType;

    return node;
}

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
    AstNode *currentNode;
    AstVariable *variables[MAX_VARIABLE_MAP_SIZE]; //NOTE: Nodes pushed onto per frame arena 

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

AstPrecendence getPrecedenceForToken(EasyToken t) {
    AstPrecendence precedence = AST_NONE;
    switch(t.type) {
        case TOKEN_SEMI_COLON: {
            precedence = AST_SAME;
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
        case TOKEN_OPEN_PARENTHESIS: {
            precedence = AST_PUSH;
        } break;
        case TOKEN_CLOSE_PARENTHESIS: {
            precedence = AST_POP;
        } break;
        default: {

        }
    }
    return precedence;
}
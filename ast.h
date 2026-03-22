enum AstPrecendence {
    AST_NONE,
    AST_PUSH,
    AST_POP_ALL,
    AST_POP,
    AST_SAME,
    AST_PLUS_MINUS,
    AST_MULTIPLY_DIVIDE,
    AST_EXPONENT,
    AST_HIGHEST_PRECEDENCE,
    
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
        case TOKEN_CARROT:
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
        } else if(precedence != AST_SAME && precedence != AST_PUSH && tree->current->precedence != AST_HIGHEST_PRECEDENCE) {
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
                tree->current->precedence = AST_SAME;
            }
        } else if(tree->current->child) {
            AstNode *parentNode = pushStruct(&globalPerFrameArena, AstNode);
            parentNode->type = AST_TYPE_PARENT;
            parentNode->precedence = tree->current->precedence;
            parentNode->parent = tree->current->parent;
            tree->current->next = parentNode;
            tree->current = parentNode;
            node->parent = parentNode;

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


void printAstNodeAdvanced(AstNode *node, char *prefix, bool isLast) {
    if (!node) return;

    // 1. Print the current indentation and the branch character
    printf("%s", prefix);
    printf(isLast ? "└── " : "├── ");

    // 2. Print the token
    if(node->type == AST_TYPE_PARENT) {
        printf("PARENT NODE: ");
        DEBUG_lexPrintToken(&node->token);
    } else {
        DEBUG_lexPrintToken(&node->token);
    }
    
    printf("\n");

    // 3. Prepare the prefix for the children
    // We append either a vertical bar or a space depending on if this node has more siblings
    char newPrefix[256]; 
    snprintf(newPrefix, sizeof(newPrefix), "%s%s", prefix, isLast ? "    " : "│   ");

    // 4. Recurse to the first child
    if (node->child) {
        AstNode *c = node->child;
        while (c != NULL) {
            // A child is the "last" if its next pointer is null
            printAstNodeAdvanced(c, newPrefix, c->next == NULL);
            c = c->next;
        }
    }
}

void printAstTree(AstTree *tree) {
    if (!tree || !tree->start) {
        printf("(Empty Tree)\n");
        return;
    }

    AstNode *node = tree->start;
    while(node) {
        // Start recursion with an empty prefix and true (since root is the only node at its level)
        printAstNodeAdvanced(node, "", true);
        node = node->next;
    }
    
}
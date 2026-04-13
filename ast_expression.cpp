enum AstExpressionPrecendence {
    AST_PRECEDENCE_NONE,
    AST_PRECEDENCE_ASSIGN,
    AST_PRECEDENCE_SUM,
    AST_PRECEDENCE_PRODUCT,
    AST_PRECEDENCE_PREFIX,
    AST_PRECEDENCE_POSTFIX,
    AST_PRECEDENCE_EXPONENT,
    AST_PRECEDENCE_CALL,
};

enum AstExpressionType {
    AST_EXPRESSION_TYPE_BLOCK,
    AST_EXPRESSION_TYPE_ASSIGN,
    AST_EXPRESSION_TYPE_LITERAL,
    AST_EXPRESSION_TYPE_NAMED,
    AST_EXPRESSION_TYPE_OPERATOR,
    AST_EXPRESSION_TYPE_PREFIX,
    AST_EXPRESSION_TYPE_POSTFIX,
    AST_EXPRESSION_TYPE_CALL,
};

struct AstExpression {
    AstExpressionType type;

    EasyToken token;
    VmOperation operation; //NOTE: easy to just add it when we run the lexer

    //---------- these are the data an expression might have depending on it's type --------- //
    AstExpression *left;
    AstExpression *right;

    List<AstExpression *> arguments; //NOTE: Resize Array
};

struct ExpressionParser {
    AstExpression *top;
    EasyTokenizer tokenizer;
    char *error;

    void logError(char *errorIn) {
        if(error) {
            error = easy_createString_printf(&globalPerFrameArena, "%s\n%s", error, errorIn);
        } else {
            error = easy_createString_printf(&globalPerFrameArena, "%s", errorIn);
        }
    }
};

bool consumeNextToken(ExpressionParser *parser, EasyTokenType typeAssumed) {
    EasyToken t = lexGetNextToken(&parser->tokenizer);
    bool wasSuccess = true;
    if(t.type != typeAssumed) {
        // assert(false);
        //NOTE: Emit error
        parser->logError(easy_createString_printf(&globalPerFrameArena, "Expected %s, got %s\n", LexTokenTypeStrings[typeAssumed], LexTokenTypeStrings[t.type]));
        wasSuccess = false;
    }
    return wasSuccess;
}

AstExpression *parseExpression(ExpressionParser *parser, int precedence);

AstExpression *parsePrefixExpression(ExpressionParser *parser, EasyToken t) {
    AstExpression *prefix = 0;
    switch(t.type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS: {
            prefix = pushStruct(&globalPerFrameArena, AstExpression);
            prefix->token = t;
            prefix->type = AST_EXPRESSION_TYPE_PREFIX;
            prefix->right = parseExpression(parser, AST_PRECEDENCE_PREFIX);
        } break;
        case TOKEN_OPEN_SQUARE_BRACKET: {
            prefix = pushStruct(&globalPerFrameArena, AstExpression);
            prefix->token = t;
            prefix->type = AST_EXPRESSION_TYPE_PREFIX;
            prefix->arguments = List<AstExpression *>::init(&globalPerFrameArena);

            bool parsing = true;
            while(parsing && lexSeeNextToken(&parser->tokenizer).type != TOKEN_CLOSE_SQUARE_BRACKET) {
                prefix->arguments.push(parseExpression(parser, 0));
                if(lexSeeNextToken(&parser->tokenizer).type != TOKEN_CLOSE_SQUARE_BRACKET) {
                    parsing = consumeNextToken(parser, TOKEN_COMMA);
                }
            }

            consumeNextToken(parser, TOKEN_CLOSE_SQUARE_BRACKET);
        } break;
        case TOKEN_WORD:
        case TOKEN_FLOAT:
        case TOKEN_INTEGER: {
            prefix = pushStruct(&globalPerFrameArena, AstExpression);
            prefix->token = t;
            prefix->type = (t.type == TOKEN_WORD) ? AST_EXPRESSION_TYPE_NAMED : AST_EXPRESSION_TYPE_LITERAL;
        } break;
        case TOKEN_OPEN_PARENTHESIS: {
            prefix = parseExpression(parser, 0);
            consumeNextToken(parser, TOKEN_CLOSE_PARENTHESIS);
        } break;
        default: {

        }
    }
    return prefix;
}

AstExpressionPrecendence getInfixPrecedenceForToken(EasyToken t) {
    AstExpressionPrecendence precedence = AST_PRECEDENCE_NONE;

    switch(t.type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS: {
            precedence = AST_PRECEDENCE_SUM;
        } break;
        case TOKEN_EQUALS: {
            precedence = AST_PRECEDENCE_ASSIGN;
        } break;
        case TOKEN_ASTRIX:
        case TOKEN_FORWARD_SLASH: {
            precedence = AST_PRECEDENCE_PRODUCT;
        } break;
        case TOKEN_CARROT: {
            precedence = AST_PRECEDENCE_EXPONENT;
        } break;
        case TOKEN_OPEN_PARENTHESIS: {
            precedence = AST_PRECEDENCE_CALL;
        } break;
        default: {

        }
    }
    return precedence;
}

int getAssociativityInfixPrecedence(EasyToken t) {
    //NOTE: Right hand associatvity operators if you have the same operator in a row say 2^3^4 it should be 2^(3^4) whereas other math operations like 2*3*4 are (2*3)*4
    int result = 0;
    if(t.type == TOKEN_CARROT) {
        result = 1;
    }
    return result;
}

AstExpression *parseInfixExpression(ExpressionParser *parser, EasyToken t, AstExpression *left) {
    AstExpression *infix = 0;
    switch(t.type) {
        case TOKEN_ASTRIX:
        case TOKEN_FORWARD_SLASH:
        case TOKEN_PLUS:
        case TOKEN_CARROT:
        case TOKEN_MINUS: {
            infix = pushStruct(&globalPerFrameArena, AstExpression);
            infix->token = t;
            infix->type = AST_EXPRESSION_TYPE_OPERATOR;
            infix->left = left;
            infix->right = parseExpression(parser, getInfixPrecedenceForToken(t) - getAssociativityInfixPrecedence(t)); //NOTE: This is for operators of the same precedence value. For example (10 - 3) - 2 is different to 10 - (3 - 2)

        } break;
        case TOKEN_EQUALS: {
            infix = pushStruct(&globalPerFrameArena, AstExpression);
            infix->token = t;
            infix->type = AST_EXPRESSION_TYPE_ASSIGN;
            infix->left = left;
            infix->right = parseExpression(parser, 0);
        } break;
        case TOKEN_OPEN_PARENTHESIS: {
            infix = pushStruct(&globalPerFrameArena, AstExpression);
            infix->token = t;
            infix->type = AST_EXPRESSION_TYPE_CALL;
            infix->left = left;
            infix->arguments = List<AstExpression *>::init(&globalPerFrameArena);

            bool parsing = true;
            while(parsing && lexSeeNextToken(&parser->tokenizer).type != TOKEN_CLOSE_PARENTHESIS) {
                AstExpression *arg  = parseExpression(parser, 0);
                infix->arguments.push(arg);

                if(lexSeeNextToken(&parser->tokenizer).type != TOKEN_CLOSE_PARENTHESIS) {
                    parsing = consumeNextToken(parser, TOKEN_COMMA);
                }
            }
            consumeNextToken(parser, TOKEN_CLOSE_PARENTHESIS);

        } break;
        default: {

        }
    }
    return infix;
}

int getPrecedenceOfNextToken(ExpressionParser *parser) {
    EasyToken nextToken = lexSeeNextToken(&parser->tokenizer);
    return (int)getInfixPrecedenceForToken(nextToken);
}

AstExpression *parseExpression(ExpressionParser *parser, int precedence) {
    EasyToken t = lexGetNextToken(&parser->tokenizer);

    AstExpression *left = parsePrefixExpression(parser, t);

    if(left) {

        while(precedence < getPrecedenceOfNextToken(parser)) {
            t = lexGetNextToken(&parser->tokenizer); //NOTE: Consume the token we just saw with the above function getPrecedenceOfNextToken
            left = parseInfixExpression(parser, t, left);
            if(!left) {
                parser->logError(easy_createString_printf(&globalPerFrameArena, "Expected an infix token, got %s", LexTokenTypeStrings[t.type]));
            }
        }
    } else {
        //NOTE: Emit error about expecting something else
        parser->logError(easy_createString_printf(&globalPerFrameArena, "Expected a prefix token, got %s", LexTokenTypeStrings[t.type]));
    }

    return left;
}



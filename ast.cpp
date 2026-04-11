
AstExpression *parseGlobalScope(char *codeToCompile, ExpressionParser *parser) {

    parser->tokenizer = lexBeginParsing(codeToCompile, EASY_LEX_OPTION_EAT_WHITE_SPACE);

    AstExpression *parent = pushStruct(&globalPerFrameArena, AstExpression);
    parent->type = AST_EXPRESSION_TYPE_BLOCK;
    parent->arguments = List<AstExpression *>::init(&globalPerFrameArena);

    do {
        AstExpression *arg  = parseExpression(parser, 0);
        parent->arguments.push(arg);
        consumeNextToken(parser, TOKEN_SEMI_COLON);
    } while(lexSeeNextToken(&parser->tokenizer).type != TOKEN_NULL_TERMINATOR);

    consumeNextToken(parser, TOKEN_NULL_TERMINATOR);
    return parent;
}

char *compileToByteCode(char *codeToCompile, List<VmOperation> *operations) {
    CompilerState *state = pushStruct(&globalPerFrameArena, CompilerState);
    initCompiler(state);
    state->operations = operations;

    AstExpression *parent = parseGlobalScope(codeToCompile, &state->parser);

    if(parent && !state->parser.error) {
        //NOTE: Now walk the ast to interpret the code to generate the Vm opcodes
        interpretExpression(state, parent);
    }

    return state->parser.error;
}
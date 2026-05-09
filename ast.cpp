
AstExpression *parseGlobalScope(char *codeToCompile, ExpressionParser *parser) {

    parser->tokenizer = lexBeginParsing(codeToCompile, EASY_LEX_OPTION_EAT_WHITE_SPACE_EXCEPT_NEW_LINE);

    AstExpression *parent = pushStruct(&globalPerFrameArena, AstExpression);
    parent->type = AST_EXPRESSION_TYPE_BLOCK;
    parent->arguments = List<AstExpression *>::init(&globalPerFrameArena);

    do {
        AstExpression *arg  = parseExpression(parser, 0);
        parent->arguments.push(arg);
        consumeNextToken(parser, TOKEN_NEWLINE);

        //NOTE: Eat any extraneous semi colons. I'm not sure if there is a more natural way this can occur that just happens by itself?
        while(lexSeeNextToken(&parser->tokenizer).type == TOKEN_SEMI_COLON) {
            lexGetNextToken(&parser->tokenizer);
        }
    } while(lexSeeNextToken(&parser->tokenizer).type != TOKEN_NULL_TERMINATOR);

    consumeNextToken(parser, TOKEN_NULL_TERMINATOR);
    return parent;
}

int getInbuiltLineCount() {
    int inbuiltLines = 0;
    char *minusCode = getInbuiltStructCode();
    while(*minusCode) {
        if(lexIsNewLine(*minusCode)) {
            inbuiltLines++;
        }
        minusCode++;
    }
    return inbuiltLines;
}

void clearPerVmRunData(ByteCodeOperations *operations, char *codeToRun, CalculatorLines *calculatorLines) {
    refreshVmMemoryArena();
    operations->clear();

    int numberOfLines = 0;
    {
        //NODE: Run through all code to find number of new lines
        char *str = codeToRun;
        while(*str) {
            if(lexIsNewLine(*str)) {
                numberOfLines++;
            }
            str++;
        }

        calculatorLines->calculatorLineCount = 0;

        //NOTE: Allocate the array
        calculatorLines->maxCalculatorLineCount = numberOfLines;
        calculatorLines->calculatorLines = pushArray(&globalPerVmRunLifetime, numberOfLines, CalculatorLine);

        //NOTE: Loop through again and set the strings
        char *start = codeToRun;
        str = codeToRun;
        int lineAt = 0;
        while(*str) {
            if(*str == '\n') {
                calculatorLines->calculatorLines[lineAt++].in = nullTerminateArena(start, (int)(str - start), &globalPerVmRunLifetime);
                start = str + 1;
            }
            str++;
        }
    }
}

char *compileToByteCode(char *codeToCompile, ByteCodeOperations *operations, CalculatorLines *calculatorLines) {
    CompilerState *state = pushStruct(&globalPerFrameArena, CompilerState);
    initCompiler(state);
    state->operations = operations;

    AstExpression *parent = parseGlobalScope(codeToCompile, &state->parser);

    if(parent && !state->parser.error) {
        //NOTE: Now walk the ast to type check the code
        typeCheckExpression(state, parent);
    }

    if(!state->parser.error) {
        //NOTE: Assume there isn't any errors now so it's safe to clear the previous data
        clearPerVmRunData(operations, codeToCompile, calculatorLines);

        if(parent && !state->parser.error) {
            //NOTE: Now walk the ast to interpret the code to generate the Vm opcodes
            interpretExpression(state, parent);
        }
    }

    return state->parser.error;
}
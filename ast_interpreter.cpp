void pushPrintOperation(CompilerState *state) {
    VmOperation data = { .type = OP_CODE_FLOAT, .as_float = (double)(state->calculatorLineAt++) };
    state->operations->push(data);

    data = { .type = OP_CODE_PRINT };
    state->operations->push(data);
}

void interpretExpression(CompilerState *state, AstExpression *expression);

void interpretLiteralExpression(CompilerState *state, AstExpression *expression) {
    if(expression->token.type == TOKEN_INTEGER) {
        VmOperation op = { .type = OP_CODE_UINT, .as_uint = expression->token.intVal };
        state->operations->push(op);
    } else if(expression->token.type == TOKEN_FLOAT) {
        VmOperation op = { .type = OP_CODE_FLOAT, .as_float = expression->token.floatVal };
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

    if(expression->left->type == AST_EXPRESSION_TYPE_ARRAY_ACCESS) {
        state->flags |= TYPE_CHECK_WRITE;
        interpretExpression(state, expression->left);
        state->flags ^= TYPE_CHECK_WRITE;

        //NOTE: Now reference for the calculator
        interpretExpression(state, expression->left);

    } else {
        assert(expression->left->type == AST_EXPRESSION_TYPE_NAMED || expression->left->type == AST_EXPRESSION_TYPE_MEMBER_ACCESS);

        char *name = nullTerminateArena(expression->left->token.at, expression->left->token.size, &globalPerFrameArena);

        AstVariable *variable = getCompilerVariable(state, name);
        assert(variable);

        VmOperation op = { .type = OP_CODE_VARIABLE_ASSIGN, .name = name };
        state->operations->push(op);

        //NOTE: For our calculator we now want to push the variable back on the VM stack so the print operation can grab it
        state->operations->push({ .type = OP_CODE_VARIABLE_REFERENCE, .as_uint = (u64)variable->type.count, .name = name });
    }
}

void interpretBlockExpression(CompilerState *state, AstExpression *expression) {
    for(int i = 0; i < expression->arguments.count; ++i) {
        interpretExpression(state, expression->arguments[i]);
        //TODO: This is specific to our calculator program. In a real programming language this wouldn't be here.
        if(expression->arguments[i]->type != AST_EXPRESSION_TYPE_STRUCT_DECLARATION) {
            pushPrintOperation(state);
        }

    }
}

void interpretArrayExpression(CompilerState *state, AstExpression *expression) {
    for(int i = expression->arguments.count - 1; i >= 0 ; --i) {
        interpretExpression(state, expression->arguments[i]);
    }
    state->operations->push({ .type = OP_CODE_NUMBER_ARRAY, .as_float = (double)expression->arguments.count });

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
        AstRecord *record = state->records.get(name);
        if(!record) {
            state->parser.error = "Function or Struct not declared";
        } else {
            state->operations->push({ .type = OP_CODE_RECORD_TYPE, .as_uint = record->totalSize});
            u8 *data = (u8 *)pushSize(&globalPerFrameArena, record->totalSize);
            state->operations->pushData(data, record->totalSize);

            //TODO: Emit byte code for initializing members
            // for(int i = 0; i < record->members.count; i++) {
            //     AstMember *member = &record->members[i];
            //     assert(member->expression);
            //     interpretExpression(state, member->expression);
            // }
        }
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
        case TOKEN_OPEN_BRACKET: {
            //NOTE: Array declaration
            interpretArrayExpression(state, expression);
        } break;
        default: {
            assert(false);
        } break;
    }
}

void interpretMemberExpression(CompilerState *state, AstExpression *expression) {
    assert(expression->left);
    assert(expression->right);
    interpretExpression(state, expression->left);
    //NOTE: Get the record and find the offset
    char *varName = nullTerminateArena(expression->left->token.at, expression->left->token.size, &globalPerFrameArena);

    AstVariable *variable = getCompilerVariable(state, varName);
    assert(variable); //NOTE: Type checker would have picked it up if this didn't exist
    assert(variable->type.name);
    AstRecord *record = state->records.get(variable->type.name);
    assert(record); //NOTE: Type checker would have picked it up if this didn't exist

    char *name = nullTerminateArena(expression->right->token.at, expression->right->token.size, &globalPerFrameArena);
    AstMember *member = ast_getMemberType(record->members, name);

    VmOperation op = { .type = OP_CODE_BYTE_OFFSET_REFERENCE, .as_uint = member->byteOffset  };
    state->operations->push(op);

}

void interpretArrayAccessExpression(CompilerState *state, AstExpression *expression) {
    assert(expression->left);
    assert(expression->right);
    interpretExpression(state, expression->left);
    //NOTE: Get the record and find the offset
    char *varName = nullTerminateArena(expression->left->token.at, expression->left->token.size, &globalPerFrameArena);

    AstVariable *variable = getCompilerVariable(state, varName);
    assert(variable); //NOTE: Type checker would have picked it up if this didn't exist

    interpretExpression(state, expression->right);

    if(!(state->flags & TYPE_CHECK_WRITE)) {
        // VmOperation op = { .type = OP_CODE_UINT, .as_uint = (u64)(variable->type.isArray ? 1 : 0 )};
        // state->operations->push(op);

        // VmOperation op1 = { .type = OP_CODE_UINT, .as_uint = (u64)variable->type.count };
        // state->operations->push(op1);
    }

    VmOperation op = { .type = (state->flags & TYPE_CHECK_WRITE) ?  OP_CODE_BYTE_OFFSET_WRITE : OP_CODE_BYTE_OFFSET_REFERENCE, .name = varName };
    state->operations->push(op);

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
        case TOKEN_FORWARD_SLASH_WITH_TILDE: {
            VmOperation op = { .type = OP_CODE_DIVIDE_INT };
            state->operations->push(op);
        } break;
        case TOKEN_CARROT: {
            VmOperation op = { .type = OP_CODE_POWER_TO };
            state->operations->push(op);
        } break;
        case TOKEN_BIT_SHIFT_RIGHT: {
            VmOperation op = { .type = OP_CODE_BIT_SHIFT_RIGHT };
            state->operations->push(op);
        } break;
        case TOKEN_BIT_SHIFT_LEFT: {
            VmOperation op = { .type = OP_CODE_BIT_SHIFT_LEFT };
            state->operations->push(op);
        } break;
        case TOKEN_BIT_AND: {
            VmOperation op = { .type = OP_CODE_BIT_OP_AND };
            state->operations->push(op);
        } break;
        case TOKEN_BIT_OR: {
            VmOperation op = { .type = OP_CODE_BIT_OP_OR };
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
        case AST_EXPRESSION_TYPE_STRUCT_DECLARATION: {
            //NOTE: Don't emit anything
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
            assert(variable); //NOTE: Type checker would have picked it up if this didn't exist

            VmOperation op = { .type = OP_CODE_VARIABLE_REFERENCE, .as_uint = (u64)variable->type.count, .name = name };
            state->operations->push(op);
        } break;
        case AST_EXPRESSION_TYPE_OPERATOR: {
            interpretOperatorExpression(state, expression);
        } break;
        case AST_EXPRESSION_TYPE_MEMBER_ACCESS: {
            interpretMemberExpression(state, expression);
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
        case AST_EXPRESSION_TYPE_ARRAY_ACCESS: {
            interpretArrayAccessExpression(state, expression);
        } break;
        default: {
            assert(false);
        } break;
    }
}
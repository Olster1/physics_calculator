StackVariable *getStackVariable(VmMachineState *state, char *name) {
    int index = getIndexForVariableMap(name);

    StackVariable *ptr = state->variables[index];

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

VmOperation vmMachine_push(VmMachineState *state, VmOperation operation) {
    VmOperation result = {};
    if((state->stackSizeBytes + sizeof(VmOperation)) <= state->stackSizeMaxBytes) {
        easyPlatform_copyMemory(state->at, &operation, sizeof(VmOperation));
        state->at += sizeof(VmOperation);
    } else {
        result.type = OP_CODE_ERROR;
        result.value_ = (int)VM_ERROR_STACK_OVERFLOW;
        assert(false);
    }
    return result;
}

void pushStackVariable(VmMachineState *state, char *name, VmOperation *values, int count) {
    StackVariable *existingVar = getStackVariable(state, name);

    if(existingVar) {
        assert(existingVar->count == count);
        VmOperation *dec = (VmOperation *)(state->stackBase + existingVar->bytesOffset);
        assert(dec->type == OP_CODE_VARIABLE_ASSIGN);
        easyPlatform_copyMemory(dec, values, sizeof(VmOperation)*count);
    } else {
         StackVariable *var = pushStruct(&globalPerFrameArena, StackVariable);
         var->count = count;

        var->name = name;
        var->bytesOffset = (u64)(state->at - state->stackBase);

        for(int i = 0; i < count; ++i) {
            vmMachine_push(state, values[i]);
        }

        int index = getIndexForVariableMap(name);

        StackVariable **ptr = &state->variables[index];
        while(*ptr) {
            ptr = &(*ptr)->next;
        }
        *ptr = var;
    }
}

VmNumberType vmOp_getValue(VmMachineState *state, VmOperation op) {
    VmNumberType result = {};
    if(op.type == OP_CODE_NUMBER) {
        result.value = op.value_;
        result.count = 1;
    } else if(op.type == OP_CODE_VARIABLE_REFERENCE) {
        assert(op.name);
        StackVariable *var = getStackVariable(state, op.name);
        assert(var);
        result.count = var->count;

        if(result.count > 1) {
            //NOTE: Is array type so load all the values on the stack
            VmOperation* start = (VmOperation *)(state->stackBase + var->bytesOffset);
            for(int i = 0; i < result.count; ++i) {
                vmMachine_push(state, start[i]);
            }
        } else {
            assert(result.count == 1);
            VmOperation *dec = (VmOperation *)(state->stackBase + var->bytesOffset);
            assert(dec->type == OP_CODE_VARIABLE_ASSIGN);
            result.value = dec->value_;
        }
    } else if(op.type == OP_CODE_NUMBER_ARRAY) {
        result.count = op.value_;
    } else {
        state->panic = "Expected a number of variable";
        assert(false);
    }
    return result;
}

bool vm_isError(VmOperation op) {
    return(op.type == OP_CODE_ERROR);
}

VmMachineState initVmMachineState(bool useRadians = true) {
    VmMachineState state = {};
    state.stackSizeMaxBytes = Megabytes(5);
    state.at = state.stackBase = (u8 *)pushSize(&globalPerFrameArena, state.stackSizeMaxBytes);
    state.useRadians = useRadians;
    return state;
}

VmOperation vmMachine_pop(VmMachineState *state) {
    VmOperation result = {};

    if (state->at > state->stackBase) {
        state->at -= sizeof(VmOperation);
        easyPlatform_copyMemory(&result, state->at, sizeof(VmOperation));
    } else {
        result.type = OP_CODE_ERROR;
        result.value_ = (int)VM_ERROR_STACK_UNDERFLOW;
        assert(false);
    }
    return result;
}

VmNumberType popAndGetValueNumber(VmMachineState *state) {
    VmNumberType result = {};
    VmOperation op = vmMachine_pop(state);
    if(!vm_isError(op)) {
        result = vmOp_getValue(state, op);
    } else {
        assert(false);
        state->panic = "error";
    }
    return result;
}

double vm_getAngle(VmMachineState *state, double value) {
    return (state->useRadians) ? value : degreesToRadians(value);
}

bool runCode(VmMachineState *state, GameState *gameState, List<VmOperation> operations, bool isUnitTest = false) {
    bool clear = false;

    for(int i = 0; i < operations.count; ++i) {
        VmOperation *op = &operations[i];

        // printf("%s\n", OpCodeTypeStrings[op->type]);

        switch (op->type) {
            // --- Vector Operations ---
            case OP_CODE_VECTOR_LENGTH: {
                double vectorSize = popAndGetValueNumber(state).count;
                VmOperation newOp = {};
                newOp.type = OP_CODE_NUMBER;

                if(vectorSize == 3) {
                    float3 a = make_float3(popAndGetValueNumber(state).value, popAndGetValueNumber(state).value, popAndGetValueNumber(state).value);
                    newOp.value_ = float3_magnitude(a);
                } else if(vectorSize == 2) {
                    float2 a = make_float2(popAndGetValueNumber(state).value, popAndGetValueNumber(state).value);
                    newOp.value_ = float2_magnitude(a);
                } else {
                    assert(false);
                }

                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_PRINT: {
                double calculatorLineNumber = popAndGetValueNumber(state).value;
                //NOTE: If unit test we want to keep the value that was going to be printed for us to check the value
                if(!isUnitTest)
                {
                    VmNumberType value = popAndGetValueNumber(state);

                    StringBuffer b = {};
                    if(value.count > 1) {
                        b.string = "[";
                        for(int i = 0; i < value.count; ++i) {
                            double arrayValue = popAndGetValueNumber(state).value;
                            b.string = easy_createString_printf(&globalPerVmRunLifetime, "%s%f, ", b.string, arrayValue);
                        }
                        b.string = easy_createString_printf(&globalPerVmRunLifetime, "%s]", b.string);
                    } else {
                        b.string = easy_createString_printf(&globalPerVmRunLifetime, "%f", value);
                    }


                    assert(gameState->calculatorLineCount < gameState->maxCalculatorLineCount);
                    assert(gameState->calculatorLineCount == calculatorLineNumber);
                    CalculatorLine *line = &gameState->calculatorLines[gameState->calculatorLineCount++];
                    line->out = b.string;
                }
                break;
            }
            case OP_CODE_DOT_PRODUCT: {
                double vectorSize = popAndGetValueNumber(state).count;
                VmOperation newOp = {};
                newOp.type = OP_CODE_NUMBER;

                if(vectorSize == 3) {
                    float3 a = make_float3(popAndGetValueNumber(state).value, popAndGetValueNumber(state).value, popAndGetValueNumber(state).value);
                    double vectorSizeB = popAndGetValueNumber(state).count;
                    assert(vectorSize = vectorSizeB);
                    float3 b = make_float3(popAndGetValueNumber(state).value, popAndGetValueNumber(state).value, popAndGetValueNumber(state).value);
                    newOp.value_ = float3_dot(a, b);
                } else if(vectorSize == 2) {
                    float2 a = make_float2(popAndGetValueNumber(state).value, popAndGetValueNumber(state).value);
                    double vectorSizeB = popAndGetValueNumber(state).count;
                    assert(vectorSize = vectorSizeB);
                    float2 b = make_float2(popAndGetValueNumber(state).value, popAndGetValueNumber(state).value);
                    newOp.value_ = float2_dot(a, b);
                }

                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_CROSS_PRODUCT: {
                 double vectorSize = popAndGetValueNumber(state).count;


                if(vectorSize == 3) {
                    float3 a = make_float3(popAndGetValueNumber(state).value, popAndGetValueNumber(state).value, popAndGetValueNumber(state).value);
                    double vectorSizeB = popAndGetValueNumber(state).count;
                    assert(vectorSize = vectorSizeB);
                    float3 b = make_float3(popAndGetValueNumber(state).value, popAndGetValueNumber(state).value, popAndGetValueNumber(state).value);
                    float3 vecRes = float3_cross(a, b);
                    vmMachine_push(state, {.type = OP_CODE_NUMBER, .value_ = vecRes.z});
                    vmMachine_push(state, {.type = OP_CODE_NUMBER, .value_ = vecRes.y});
                    vmMachine_push(state, {.type = OP_CODE_NUMBER, .value_ = vecRes.x});
                    vmMachine_push(state, {.type = OP_CODE_NUMBER_ARRAY, .value_ = 3});
                } else if(vectorSize == 2) {
                    float2 a = make_float2(popAndGetValueNumber(state).value, popAndGetValueNumber(state).value);
                    double vectorSizeB = popAndGetValueNumber(state).count;
                    assert(vectorSize = vectorSizeB);
                    float2 b = make_float2(popAndGetValueNumber(state).value, popAndGetValueNumber(state).value);
                    float vecRes = float2_cross(a, b);
                    vmMachine_push(state, {.type = OP_CODE_NUMBER, .value_ = vecRes});
                }

                break;
            }
            case OP_CODE_SUMMATION: {
                double arrayLength = popAndGetValueNumber(state).count;

                double total = 0;
                for(int i = 0; i < arrayLength; ++i) {
                    total += popAndGetValueNumber(state).value;
                }

                VmOperation newOp = { .type = OP_CODE_NUMBER, .value_ = total};
                vmMachine_push(state, newOp);
                break;
            }
            // --- Trigonometry ---
            case OP_CODE_SIN: {
                double value = popAndGetValueNumber(state).value;
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = sin(vm_getAngle(state, value));

                vmMachine_push(state, newOp);
                break;
            }

            case OP_CODE_COS: {
                double value = popAndGetValueNumber(state).value;
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = cos(vm_getAngle(state, value));
                vmMachine_push(state, newOp);
                break;
            }
             case OP_CODE_NEGATE: {
                double value = popAndGetValueNumber(state).value;
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = -1*value;
                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_TAN: {
                double value = popAndGetValueNumber(state).value;
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = tan(vm_getAngle(state, value));
                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_ARCSIN: {
                double value = popAndGetValueNumber(state).value;
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = asin(vm_getAngle(state, value));
                vmMachine_push(state, newOp);
                break;
            }
             case OP_CODE_SQR: {
                double value = popAndGetValueNumber(state).value;
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = value * value;
                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_SQRT: {
                double value = popAndGetValueNumber(state).value;
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = sqrt(value);
                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_ARCCOS: {
                double value = popAndGetValueNumber(state).value;
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = acos(vm_getAngle(state, value));
                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_ARCTAN: {
                double value = popAndGetValueNumber(state).value;
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = atan(vm_getAngle(state, value));
                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_ARCTAN2: {
                double value = popAndGetValueNumber(state).value;
                double value1 = popAndGetValueNumber(state).value;
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = atan2(vm_getAngle(state, value1), vm_getAngle(state, value));
                vmMachine_push(state, newOp);
                break;
            }

            // --- Configuration ---
            case OP_CODE_SET_DEGREES_MODE: {
                state->useRadians = false;
                gameState->useRadians = false;
                break;
            }
            case OP_CODE_SET_RADIANS_MODE: {
                state->useRadians = true;
                gameState->useRadians = true;
                break;
            }

            // --- Arithmetic ---
            case OP_CODE_ADD: {
                VmOperation op = vmMachine_pop(state);
                VmOperation op1 = vmMachine_pop(state);
                if(!vm_isError(op) && !vm_isError(op1)) {
                    VmOperation newOp = {};
                    newOp.type = OP_CODE_NUMBER;
                    newOp.value_ = vmOp_getValue(state, op).value + vmOp_getValue(state, op1).value;
                    vmMachine_push(state, newOp);
                    // printf("%f\n", newOp.value_);
                }
                break;
            }
            case OP_CODE_MINUS: {
                VmOperation op = vmMachine_pop(state);
                VmOperation op1 = vmMachine_pop(state);
                if(!vm_isError(op) && !vm_isError(op1)) {
                    VmOperation newOp = {};
                    newOp.type = OP_CODE_NUMBER;
                    newOp.value_ = vmOp_getValue(state, op1).value - vmOp_getValue(state, op).value;
                    vmMachine_push(state, newOp);
                    // printf("%f\n", newOp.value_);
                }
                break;
            }
            case OP_CODE_MULTIPLY: {
                 VmOperation op = vmMachine_pop(state);
                VmOperation op1 = vmMachine_pop(state);
                if(!vm_isError(op) && !vm_isError(op1)) {
                    VmOperation newOp = {};
                    newOp.type = OP_CODE_NUMBER;
                    newOp.value_ = vmOp_getValue(state, op).value * vmOp_getValue(state, op1).value;
                    vmMachine_push(state, newOp);
                    // printf("%f\n", newOp.value_);
                }
                break;
            }
            case OP_CODE_DIVIDE: {
                 VmOperation op = vmMachine_pop(state);
                VmOperation op1 = vmMachine_pop(state);
                if(!vm_isError(op) && !vm_isError(op1)) {
                    VmOperation newOp = {};
                    newOp.type = OP_CODE_NUMBER;
                    newOp.value_ = vmOp_getValue(state, op1).value / vmOp_getValue(state, op).value;
                    vmMachine_push(state, newOp);
                    // printf("%f\n", newOp.value_);
                }
                break;
            }
              case OP_CODE_POWER_TO: {
                VmOperation op = vmMachine_pop(state);
                VmOperation op1 = vmMachine_pop(state);
                if(!vm_isError(op) && !vm_isError(op1)) {
                    VmOperation newOp = {};
                    newOp.type = OP_CODE_NUMBER;
                    newOp.value_ = pow(vmOp_getValue(state, op1).value, vmOp_getValue(state, op).value);
                    vmMachine_push(state, newOp);
                    // printf("%f\n", newOp.value_);
                }
                break;
            }
            case OP_CODE_CLEAR: {
                //NOTE: Clear the old code
                clearCalculatorBuffer(gameState);
                clear = true;

                break;
            }

            // --- Literals / Types ---

            case OP_CODE_NUMBER: {
                vmMachine_push(state, *op);
                break;
            }
            case OP_CODE_NUMBER_ARRAY: {
                vmMachine_push(state, *op);
                break;
            }
            case OP_CODE_VARIABLE_REFERENCE: {
                vmMachine_push(state, *op);
                break;
            }
            case OP_CODE_VARIABLE_ASSIGN: {
                VmNumberType value = popAndGetValueNumber(state);

                if(value.count > 1) {
                    VmOperation *tempArray = pushArray(&globalPerFrameArena, value.count, VmOperation);
                    for(int i = 0; i < value.count; ++i) {
                        tempArray[i].type = OP_CODE_VARIABLE_ASSIGN;
                        tempArray[i].value_ = popAndGetValueNumber(state).value;
                    }
                    pushStackVariable(state, op->name, tempArray, value.count);
                } else {
                    VmOperation opcode = { .type = OP_CODE_VARIABLE_ASSIGN, .value_ = value.value };
                    pushStackVariable(state, op->name, &opcode, 1);
                }

                break;
            }

            default: {
                // Error handling here
                break;
            }
        }
    }
    return clear;
}
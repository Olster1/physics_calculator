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
        result.as_int = (int)VM_ERROR_STACK_OVERFLOW;
        assert(false);
    }
    return result;
}

void pushStackVariable(VmMachineState *state, char *name, VmOperation *values, int count) {
    StackVariable *existingVar = getStackVariable(state, name);

    if(existingVar) {
        if(existingVar->count == count) {
            //NOTE: Just the same values so just memcpy them from one spot to the other
            VmOperation *dec = (VmOperation *)(state->stackBase + existingVar->bytesOffset);
            assert(dec->type == OP_CODE_FLOAT || dec->type == OP_CODE_UINT);
            easyPlatform_copyMemory(dec, values, sizeof(VmOperation)*count);
        } else {
            //NOTE: Changing size of array so reallocate the array on the stack
             existingVar->count = count;
             existingVar->bytesOffset = (u64)(state->at - state->stackBase);
             for(int i = 0; i < count; ++i) {
                vmMachine_push(state, values[i]);
            }
        }
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

VmNumberType vmOp_getValue(VmMachineState *state, VmOperation op, OpCode desiredType) {
    VmNumberType result = {};
    if(op.type == OP_CODE_UINT) {
        result.type = op.type;
        result.raw = op.raw;
        if(desiredType == OP_CODE_FLOAT) {
            result.as_float = (double)op.as_uint;
            result.type = OP_CODE_FLOAT;
        } else if(desiredType == OP_CODE_UINT){
            result.as_uint = op.as_uint;
            result.type = OP_CODE_UINT;
        }
        result.count = 1;
    } else if(op.type == OP_CODE_FLOAT) {
        result.type = op.type;
        result.raw = op.raw;
        if(desiredType == OP_CODE_FLOAT) {
            result.as_float = op.as_float;
            result.type = OP_CODE_FLOAT;
        } else if(desiredType == OP_CODE_UINT) {
            result.as_uint = (u64)op.as_float;
            result.type = OP_CODE_FLOAT;
        }
        result.count = 1;
    } else if(op.type == OP_CODE_VARIABLE_REFERENCE) {
        assert(op.name);
        StackVariable *var = getStackVariable(state, op.name);
        assert(var);
        result.count = var->count;
        result.type = OP_CODE_FLOAT;

        if(result.count > 1) {
            //NOTE: Is array type so load all the values on the stack
            VmOperation* start = (VmOperation *)(state->stackBase + var->bytesOffset);
            for(int i = 0; i < result.count; ++i) {
                vmMachine_push(state, start[i]);
            }
        } else {
            assert(result.count == 1);
            VmOperation *dec = (VmOperation *)(state->stackBase + var->bytesOffset);
            assert(dec->type == OP_CODE_FLOAT || dec->type == OP_CODE_UINT);
            result = vmOp_getValue(state, *dec, desiredType);
        }
    } else if(op.type == OP_CODE_NUMBER_ARRAY) {
        // assert(op.type == OP_CODE_FLOAT);
        result.type = OP_CODE_FLOAT;
        result.count = (double)op.as_float;
    } else if(op.type == OP_CODE_STRING) {
        result.type = OP_CODE_STRING;
        result.name = op.name;
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
        result.as_uint = (int)VM_ERROR_STACK_UNDERFLOW;
        assert(false);
    }
    return result;
}

VmNumberType popAndGetValueNumber(VmMachineState *state, OpCode desiredType) {
    VmNumberType result = {};
    VmOperation op = vmMachine_pop(state);
    if(!vm_isError(op)) {
        result = vmOp_getValue(state, op, desiredType);
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
                double vectorSize = popAndGetValueNumber(state, OP_CODE_FLOAT).count;
                VmOperation newOp = {};
                newOp.type = OP_CODE_FLOAT;

                if(vectorSize == 3) {
                    float3 a = make_float3(popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float);
                    newOp.as_float = float3_magnitude(a);
                } else if(vectorSize == 2) {
                    float2 a = make_float2(popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float);
                    newOp.as_float = float2_magnitude(a);
                } else {
                    assert(false);
                }

                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_PRINT: {
                double calculatorLineNumber = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                //NOTE: If unit test we want to keep the value that was going to be printed for us to check the value
                if(!isUnitTest)
                {
                    VmNumberType value = popAndGetValueNumber(state, OP_CODE_NONE);

                    StringBuffer b = {};
                    if(value.count > 1) {
                        assert(!value.name);
                        b.string = "[";
                        for(int i = 0; i < value.count; ++i) {
                            double arrayValue = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                            b.string = easy_createString_printf(&globalPerVmRunLifetime, "%s%f", b.string, arrayValue);
                            if(i < value.count - 1) {
                                b.string = easy_createString_printf(&globalPerVmRunLifetime, "%s, ", b.string);
                            }
                        }
                        b.string = easy_createString_printf(&globalPerVmRunLifetime, "%s]", b.string);
                    } else {
                        if(value.name) {
                            b.string = easy_createString_printf(&globalPerVmRunLifetime, "%s", value.name);
                        } else if(value.type == OP_CODE_FLOAT){
                            b.string = easy_createString_printf(&globalPerVmRunLifetime, "%f", value.as_float);
                        } else if(value.type == OP_CODE_UINT){
                            b.string = easy_createString_printf(&globalPerVmRunLifetime, "%lu", value.as_int);
                        } else {
                            assert(false);
                        }

                    }


                    assert(gameState->calculatorLinesParent.calculatorLineCount < gameState->calculatorLinesParent.maxCalculatorLineCount);
                    assert(gameState->calculatorLinesParent.calculatorLineCount == calculatorLineNumber);
                    CalculatorLine *line = &gameState->calculatorLinesParent.calculatorLines[gameState->calculatorLinesParent.calculatorLineCount++];
                    line->out = b.string;
                }
                break;
            }
            case OP_CODE_DOT_PRODUCT: {
                double vectorSize = popAndGetValueNumber(state, OP_CODE_FLOAT).count;
                VmOperation newOp = {};
                newOp.type = OP_CODE_FLOAT;

                if(vectorSize == 3) {
                    float3 a = make_float3(popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float);
                    double vectorSizeB = popAndGetValueNumber(state, OP_CODE_FLOAT).count;
                    assert(vectorSize = vectorSizeB);
                    float3 b = make_float3(popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float);
                    newOp.as_float = float3_dot(a, b);
                } else if(vectorSize == 2) {
                    float2 a = make_float2(popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float);
                    double vectorSizeB = popAndGetValueNumber(state, OP_CODE_FLOAT).count;
                    assert(vectorSize = vectorSizeB);
                    float2 b = make_float2(popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float);
                    newOp.as_float = float2_dot(a, b);
                }

                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_CROSS_PRODUCT: {
                 double vectorSize = popAndGetValueNumber(state, OP_CODE_FLOAT).count;


                if(vectorSize == 3) {
                    float3 a = make_float3(popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float);
                    double vectorSizeB = popAndGetValueNumber(state, OP_CODE_FLOAT).count;
                    assert(vectorSize = vectorSizeB);
                    float3 b = make_float3(popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float);
                    float3 vecRes = float3_cross(a, b);
                    vmMachine_push(state, {.type = OP_CODE_FLOAT, .as_float = vecRes.z});
                    vmMachine_push(state, {.type = OP_CODE_FLOAT, .as_float = vecRes.y});
                    vmMachine_push(state, {.type = OP_CODE_FLOAT, .as_float = vecRes.x});
                    vmMachine_push(state, {.type = OP_CODE_NUMBER_ARRAY, .as_float = 3});
                } else if(vectorSize == 2) {
                    float2 a = make_float2(popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float);
                    double vectorSizeB = popAndGetValueNumber(state, OP_CODE_FLOAT).count;
                    assert(vectorSize = vectorSizeB);
                    float2 b = make_float2(popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float);
                    float vecRes = float2_cross(a, b);
                    vmMachine_push(state, {.type = OP_CODE_FLOAT, .as_float = vecRes});
                }

                break;
            }
            case OP_CODE_SUMMATION: {
                double arrayLength = popAndGetValueNumber(state, OP_CODE_FLOAT).count;

                double total = 0;
                for(int i = 0; i < arrayLength; ++i) {
                    total += popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                }

                VmOperation newOp = { .type = OP_CODE_FLOAT, .as_float = total};
                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_QUADRATIC: {
                double c = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                double b = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                double a = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;

                double underSqrt = (b*b) - 4*a*c;

                if(underSqrt < 0) {
                    //NOTE: No solutions
                }

                double bottom = 2*a;
                double resultA = (-b - sqrt(underSqrt)) / bottom;
                double resultB = (-b + sqrt(underSqrt)) / bottom;

                vmMachine_push(state, { .type = OP_CODE_FLOAT, .as_float = resultA });
                vmMachine_push(state, { .type = OP_CODE_FLOAT, .as_float = resultB });
                vmMachine_push(state, { .type = OP_CODE_NUMBER_ARRAY, .as_float = 2 });
                break;
            }

            // --- Trigonometry ---
            case OP_CODE_SIN: {
                double value = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                VmOperation newOp = { .type = OP_CODE_FLOAT };
                newOp.as_float = sin(vm_getAngle(state, value));

                vmMachine_push(state, newOp);
                break;
            }

            case OP_CODE_COS: {
                double value = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                VmOperation newOp = { .type = OP_CODE_FLOAT };
                newOp.as_float = cos(vm_getAngle(state, value));
                vmMachine_push(state, newOp);
                break;
            }
             case OP_CODE_NEGATE: {
                double value = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                VmOperation newOp = { .type = OP_CODE_FLOAT };
                newOp.as_float = -1*value;
                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_TAN: {
                double value = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                VmOperation newOp = { .type = OP_CODE_FLOAT };
                newOp.as_float = tan(vm_getAngle(state, value));
                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_ARCSIN: {
                double value = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                VmOperation newOp = { .type = OP_CODE_FLOAT };
                newOp.as_float = asin(vm_getAngle(state, value));
                vmMachine_push(state, newOp);
                break;
            }
             case OP_CODE_SQR: {
                double value = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                VmOperation newOp = { .type = OP_CODE_FLOAT} ;
                newOp.as_float = value * value;
                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_SQRT: {
                double value = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                VmOperation newOp = { .type = OP_CODE_FLOAT };
                newOp.as_float = sqrt(value);
                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_ARCCOS: {
                double value = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                VmOperation newOp = { .type = OP_CODE_FLOAT };
                newOp.as_float = acos(vm_getAngle(state, value));
                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_ARCTAN: {
                double value = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                VmOperation newOp = { .type = OP_CODE_FLOAT };
                newOp.as_float = atan(vm_getAngle(state, value));
                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_ARCTAN2: {
                double value = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                double value1 = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                VmOperation newOp = { .type = OP_CODE_FLOAT};
                newOp.as_float = atan2(vm_getAngle(state, value1), vm_getAngle(state, value));
                vmMachine_push(state, newOp);
                break;
            }

            // --- Configuration ---
            case OP_CODE_SET_DEGREES_MODE: {
                state->useRadians = false;
                gameState->settingsToSave.useRadians = false;
                vmMachine_push(state, { .type = OP_CODE_STRING, .name = "Degrees Set" });
                break;
            }
            case OP_CODE_SET_RADIANS_MODE: {
                state->useRadians = true;
                gameState->settingsToSave.useRadians = true;
                vmMachine_push(state, { .type = OP_CODE_STRING, .name = "Radians Set" });
                break;
            }

            // --- Arithmetic ---
            case OP_CODE_ADD: {
                VmOperation op = vmMachine_pop(state);
                VmOperation op1 = vmMachine_pop(state);
                if(!vm_isError(op) && !vm_isError(op1)) {
                    VmOperation newOp = {};

                    //TODO: This could be a different instruction for float vs int ADD
                    newOp.type = OP_CODE_FLOAT;
                    newOp.as_float = vmOp_getValue(state, op, OP_CODE_FLOAT).as_float + vmOp_getValue(state, op1, OP_CODE_FLOAT).as_float;
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
                    newOp.type = OP_CODE_FLOAT;
                    newOp.as_float = vmOp_getValue(state, op1, OP_CODE_FLOAT).as_float - vmOp_getValue(state, op, OP_CODE_FLOAT).as_float;
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
                    newOp.type = OP_CODE_FLOAT;
                    newOp.as_float = vmOp_getValue(state, op, OP_CODE_FLOAT).as_float * vmOp_getValue(state, op1, OP_CODE_FLOAT).as_float;
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
                    newOp.type = OP_CODE_FLOAT;
                    newOp.as_float = vmOp_getValue(state, op1, OP_CODE_FLOAT).as_float / vmOp_getValue(state, op, OP_CODE_FLOAT).as_float;
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
                    newOp.type = OP_CODE_FLOAT;
                    newOp.as_float = pow(vmOp_getValue(state, op1, OP_CODE_FLOAT).as_float, vmOp_getValue(state, op, OP_CODE_FLOAT).as_float);
                    vmMachine_push(state, newOp);
                    // printf("%f\n", newOp.value_);
                }
                break;
            }
            case OP_CODE_BIT_SHIFT_LEFT: {
                VmOperation op = vmMachine_pop(state);
                VmOperation op1 = vmMachine_pop(state);
                if(!vm_isError(op) && !vm_isError(op1)) {
                    VmOperation newOp = {};
                    newOp.type = OP_CODE_UINT;
                    newOp.as_uint =((u64)(vmOp_getValue(state, op1, OP_CODE_UINT).as_uint) << (u64)(vmOp_getValue(state, op, OP_CODE_UINT).as_uint));
                    vmMachine_push(state, newOp);
                    // printf("%f\n", newOp.value_);
                }
                break;
            }
            case OP_CODE_BIT_SHIFT_RIGHT: {
                VmOperation op = vmMachine_pop(state);
                VmOperation op1 = vmMachine_pop(state);
                if(!vm_isError(op) && !vm_isError(op1)) {
                    VmOperation newOp = {};
                    newOp.type = OP_CODE_UINT;
                    newOp.as_uint = ((u64)(vmOp_getValue(state, op1, OP_CODE_UINT).as_uint) >> (u64)(vmOp_getValue(state, op, OP_CODE_UINT).as_uint));
                    vmMachine_push(state, newOp);
                    // printf("%f\n", newOp.value_);
                }
                break;
            }
            case OP_CODE_BIT_OP_AND: {
                VmOperation op = vmMachine_pop(state);
                VmOperation op1 = vmMachine_pop(state);
                if(!vm_isError(op) && !vm_isError(op1)) {
                    VmOperation newOp = {};
                    newOp.type = OP_CODE_UINT;
                    newOp.as_uint =((u64)(vmOp_getValue(state, op1, OP_CODE_UINT).as_uint) & (u64)(vmOp_getValue(state, op, OP_CODE_UINT).as_uint));
                    vmMachine_push(state, newOp);
                    // printf("%f\n", newOp.value_);
                }
                break;
            }
             case OP_CODE_BIT_OP_OR: {
                VmOperation op = vmMachine_pop(state);
                VmOperation op1 = vmMachine_pop(state);
                if(!vm_isError(op) && !vm_isError(op1)) {
                    VmOperation newOp = {};
                    newOp.type = OP_CODE_UINT;
                    newOp.as_uint = ((u64)(vmOp_getValue(state, op1, OP_CODE_UINT).as_uint) | (u64)(vmOp_getValue(state, op, OP_CODE_UINT).as_uint));
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
            case OP_CODE_FLOAT: {
                vmMachine_push(state, *op);
                break;
            }
            case OP_CODE_UINT: {
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
                VmNumberType value = popAndGetValueNumber(state, OP_CODE_NONE);

                if(value.count > 1) {
                    VmOperation *tempArray = pushArray(&globalPerFrameArena, value.count, VmOperation);
                    for(int i = value.count - 1; i >= 0; --i) {
                        VmNumberType arrayValue = popAndGetValueNumber(state, OP_CODE_NONE);
                        tempArray[i].type = arrayValue.type;
                        tempArray[i].raw = arrayValue.raw;
                    }
                    pushStackVariable(state, op->name, tempArray, value.count);
                } else {
                    VmOperation opcode = { .type = value.type, .raw = value.raw };
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
char *print_binary_formatted(u64 n) {
    if (n == 0) {
        return easy_createString_printf(&globalPerFrameArena, "0000");
    }

    int total_bits = sizeof(n) * 8;
    int highest_bit = 0;

    // 1. Find the highest set bit
    for (int i = total_bits - 1; i >= 0; i--) {
        if ((n >> i) & 1) {
            highest_bit = i;
            break;
        }
    }

    // 2. Round up to the start of the 4-bit nibble boundary
    int start_bit = ((highest_bit / 4) + 1) * 4 - 1;

    char *result = "";

    // 3. Build the string using your arena-based printf
    for (int i = start_bit; i >= 0; i--) {
        int bit = (n >> i) & 1;

        result = easy_createString_printf(&globalPerFrameArena, "%s%d", result, bit);

        // Add space after every 4 bits, except the very last bit
        if (i % 4 == 0 && i != 0) {
            result = easy_createString_printf(&globalPerFrameArena, "%s  ", result);
        }
    }

    return result;
}

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

VmOperation vmMachine_pushData(VmMachineState *state, u8 *data, u64 sizeToPush) {
    VmOperation result = {};
    if((state->stackSizeBytes + sizeToPush) <= state->stackSizeMaxBytes) {
        easyPlatform_copyMemory(state->at, data, sizeToPush);
        state->at += sizeToPush;
    } else {
        result.type = OP_CODE_ERROR;
        result.as_int = (int)VM_ERROR_STACK_OVERFLOW;
        assert(false);
    }
    return result;
}

void pushStackVariable(VmMachineState *state, char *name, VmOperation *values, int count, OpCode type, u64 structSize) {
    StackVariable *existingVar = getStackVariable(state, name);

    if(existingVar) {
        if(existingVar->count == count) {
            //NOTE: Just the same values so just memcpy them from one spot to the other
            VmOperation *dec = (VmOperation *)(state->stackBase + existingVar->bytesOffset);
            assert(dec->type == OP_CODE_FLOAT || dec->type == OP_CODE_UINT);
            easyPlatform_copyMemory(dec, values, sizeof(VmOperation)*count);
            existingVar->type = type;

        } else {
            //NOTE: Changing size of array so reallocate the array on the stack
             existingVar->count = count;
             existingVar->bytesOffset = (u64)(state->at - state->stackBase);
             for(int i = 0; i < count; ++i) {
                vmMachine_push(state, values[i]);
            }
            existingVar->type = type;
            existingVar->structSize = structSize;
        }
    } else {
         StackVariable *var = pushStruct(&globalPerFrameArena, StackVariable);
         var->count = count;
         var->type = type;

        var->name = name;
        var->structSize = structSize;
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

VmNumberType vmOp_getValue(VmMachineState *state, VmOperation op, OpCode desiredType);

VmNumberType getValueFromStack(VmMachineState *state, u64 offset, OpCode desiredType) {
    VmOperation *memAt = (VmOperation *)(state->stackBase + offset);

    VmNumberType result = vmOp_getValue(state, *memAt, desiredType);
    return result;
}

void setValueFromStack(VmMachineState *state, u64 offset, VmNumberType numberType) {
    VmOperation *memAt = (VmOperation *)(state->stackBase + offset);
    memAt->type = numberType.type;
    memAt->raw = numberType.raw;
}

VmNumberType popAndGetValueNumber(VmMachineState *state, OpCode desiredType);
VmNumberType vmOp_getValue(VmMachineState *state, VmOperation op, OpCode desiredType) {
    VmNumberType result = {};

    if(op.type == OP_CODE_RECORD_TYPE) {
        result.type = op.type;
        result.raw = op.raw;
    } else if(op.type == OP_CODE_UINT) {
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
    } else if(op.type == OP_CODE_VARIABLE_REFERENCE_OFFSET) {
        u64 isArray = popAndGetValueNumber(state, OP_CODE_UINT).as_uint;
        result.count = popAndGetValueNumber(state, OP_CODE_UINT).as_uint;

        if(result.count > 1 || (isArray == 1)) {
            result.type = OP_CODE_NUMBER_ARRAY;
            //NOTE: Is array type so load all the values on the stack
            VmOperation* start = (VmOperation *)(state->stackBase + op.as_uint);
            for(int i = 0; i < result.count; ++i) {
                vmMachine_push(state, start[i]);
            }
        } else {
            assert(result.count == 1);
            VmNumberType dec = getValueFromStack(state, op.as_uint, OP_CODE_NONE);
            assert(dec.type == OP_CODE_FLOAT || dec.type == OP_CODE_UINT);
            VmOperation vmOp = { .type = dec.type, .raw = dec.raw };
            result = vmOp_getValue(state, vmOp, desiredType);
        }
    } else if(op.type == OP_CODE_NUMBER_ARRAY) {
        result.type = OP_CODE_NUMBER_ARRAY;
        result.count = (double)op.as_float;
    } else if(op.type == OP_CODE_STRING) {
        result.type = OP_CODE_STRING;
        result.name = op.name;
    } else {
        state->panic = "Expected a number of variable";
        assert(false);
    }
    result.printFormat = op.printFormat;
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

u8 *vmMachine_popData(VmMachineState *state, u64 sizeToPop) {
    u8 *result = 0;

    if (state->at > state->stackBase) {
        state->at -= sizeToPop;
        result = (u8 *)pushSize(&globalPerFrameArena, sizeToPop);
        easyPlatform_copyMemory(result, state->at, sizeToPop);
    } else {
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

double vm_radiansToDegrees(VmMachineState *state, double value) {
    return (state->useRadians) ? value : radiansToDegrees(value);
}

double vm_getLiteralValueAsFloat(VmNumberType numberType) {
    double result = 0;
    if(numberType.type == OP_CODE_FLOAT) {
        result = numberType.as_float;
    } else if(numberType.type == OP_CODE_UINT) {
        result = (double)numberType.as_int;
    } else {
        assert(false);
    }
    return result;
}

bool runCode(VmMachineState *state, GameState *gameState, ByteCodeOperations *operations, bool isUnitTest = false) {
    bool clear = false;

    u8 *at = operations->operations;
    while(((uintptr_t)at - ((uintptr_t)operations->operations)) < operations->totalSize) {
        VmOperation *op = (VmOperation *)at;

        // printf("%s\n", OpCodeTypeStrings[op->type]);

        u64 sizeToMove = sizeof(VmOperation);

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
            case OP_CODE_VECTOR_NORMALIZE: {
                double vectorSize = popAndGetValueNumber(state, OP_CODE_FLOAT).count;

                if(vectorSize == 3) {
                    float3 a = make_float3(popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float);
                    float3 vec = normalize_float3(a);

                    vmMachine_push(state, { .type = OP_CODE_FLOAT, .as_float = vec.z });
                    vmMachine_push(state, { .type = OP_CODE_FLOAT, .as_float = vec.y });
                    vmMachine_push(state, { .type = OP_CODE_FLOAT, .as_float = vec.x });
                    vmMachine_push(state, { .type = OP_CODE_NUMBER_ARRAY, .as_float = 3 });

                } else if(vectorSize == 2) {
                    float2 a = make_float2(popAndGetValueNumber(state, OP_CODE_FLOAT).as_float, popAndGetValueNumber(state, OP_CODE_FLOAT).as_float);
                    float2 vec = normalize_float2(a);

                    vmMachine_push(state, { .type = OP_CODE_FLOAT, .as_float = vec.y });
                    vmMachine_push(state, { .type = OP_CODE_FLOAT, .as_float = vec.x });
                    vmMachine_push(state, { .type = OP_CODE_NUMBER_ARRAY, .as_float = 2 });
                } else {
                    assert(false);
                }

                break;
            }
            case OP_CODE_PRINT: {
                double calculatorLineNumber = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                //NOTE: If unit test we want to keep the value that was going to be printed for us to check the value
                if(!isUnitTest)
                {
                    VmNumberType value = popAndGetValueNumber(state, OP_CODE_NONE);

                    assert(gameState->calculatorLinesParent.calculatorLineCount < gameState->calculatorLinesParent.maxCalculatorLineCount);
                    assert(gameState->calculatorLinesParent.calculatorLineCount == calculatorLineNumber);
                    CalculatorLine *line = &gameState->calculatorLinesParent.calculatorLines[gameState->calculatorLinesParent.calculatorLineCount++];

                    if(value.printFormat == VM_PRINT_FORMAT_COLOR) {
                        line->colorOut = color_hexARGBTo01(value.as_int);
                    } else {

                        StringBuffer b = {};
                        b.string = "";
                        if(value.type == OP_CODE_NUMBER_ARRAY || value.count > 1) {
                            assert(!value.name);
                            b.string = "[";
                            //NOTE: Pull out the values first as they go from top of stack to bottom, then print them in reverse sp they're in the right order
                            double *tempArray = pushArray(&globalPerFrameArena, value.count, double);
                            for(int i = 0; i < value.count; ++i) {
                                double arrayValue = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                                tempArray[(value.count - 1) - i] = arrayValue;
                            }

                            for(int i = 0; i < value.count; ++i) {
                                double arrayValue = tempArray[i];
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
                                if(value.printFormat == VM_PRINT_FORMAT_BINARY) {
                                    b.string = easy_createString_printf(&globalPerVmRunLifetime, "%s", print_binary_formatted(value.as_int));
                                } else if(value.printFormat == VM_PRINT_FORMAT_HEX) {
                                    b.string = easy_createString_printf(&globalPerVmRunLifetime, "0x%x", value.as_int);
                                } else {
                                    b.string = easy_createString_printf(&globalPerVmRunLifetime, "%lu", value.as_int);
                                }
                            } else if(value.type == OP_CODE_RECORD_TYPE) {
                                //TODO: Record so print all the value types
                                u8 *data = vmMachine_popData(state, value.as_uint);
                                int h = 0;

                            } else {
                                assert(false);
                            }

                        }

                        line->out = b.string;
                    }
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
            case OP_CODE_PRINT_AS_BINARY: {
                VmOperation op = vmMachine_pop(state);
                op.printFormat = VM_PRINT_FORMAT_BINARY;
                vmMachine_push(state, op);
            } break;
            case OP_CODE_PRINT_AS_COLOR: {
                VmOperation op = vmMachine_pop(state);
                op.printFormat = VM_PRINT_FORMAT_COLOR;
                vmMachine_push(state, op);
            } break;
            case OP_CODE_PRINT_AS_HEXADECIMAL: {
                VmOperation op = vmMachine_pop(state);
                op.printFormat = VM_PRINT_FORMAT_HEX;
                vmMachine_push(state, op);
            } break;
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
             case OP_CODE_ARCSIN: {
                double value = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                VmOperation newOp = { .type = OP_CODE_FLOAT };
                newOp.as_float =  vm_radiansToDegrees(state, asin(value));
                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_ARCCOS: {
                double value = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                VmOperation newOp = { .type = OP_CODE_FLOAT };
                newOp.as_float = vm_radiansToDegrees(state, acos(value));
                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_ARCTAN: {
                double value = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                VmOperation newOp = { .type = OP_CODE_FLOAT };
                newOp.as_float = vm_radiansToDegrees(state, atan(value));
                vmMachine_push(state, newOp);
                break;
            }
            case OP_CODE_ARCTAN2: {
                double value = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                double value1 = popAndGetValueNumber(state, OP_CODE_FLOAT).as_float;
                VmOperation newOp = { .type = OP_CODE_FLOAT};
                newOp.as_float = vm_radiansToDegrees(state, atan2(value1, value));
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
                VmNumberType op = popAndGetValueNumber(state, OP_CODE_NONE);
                VmNumberType op1 = popAndGetValueNumber(state, OP_CODE_NONE);

                VmOperation newOp = {};
                if(op.type == OP_CODE_FLOAT || op1.type == OP_CODE_FLOAT) {
                    //NOTE: Automatic casts to float
                    newOp.type = OP_CODE_FLOAT;
                    newOp.as_float = vm_getLiteralValueAsFloat(op) + vm_getLiteralValueAsFloat(op1);
                    vmMachine_push(state, newOp);
                } else if(op.type == OP_CODE_UINT && op1.type == OP_CODE_UINT) {
                    newOp.type = OP_CODE_UINT;
                    newOp.as_int = op.as_int + op1.as_int;
                    vmMachine_push(state, newOp);
                } else {
                    printf("%s %s\n",OpCodeTypeStrings[op.type], OpCodeTypeStrings[op1.type]);
                    assert(false);
                }
                break;
            }
            case OP_CODE_MINUS: {
                VmNumberType op = popAndGetValueNumber(state, OP_CODE_NONE);
                VmNumberType op1 = popAndGetValueNumber(state, OP_CODE_NONE);
                VmOperation newOp = {};
                if(op.type == OP_CODE_FLOAT || op1.type == OP_CODE_FLOAT) {
                    newOp.type = OP_CODE_FLOAT;
                    newOp.as_float = vm_getLiteralValueAsFloat(op1) - vm_getLiteralValueAsFloat(op);
                    vmMachine_push(state, newOp);
                } else if(op.type == OP_CODE_UINT && op1.type == OP_CODE_UINT) {
                    newOp.type = OP_CODE_UINT;
                    newOp.as_int = op.as_int - op1.as_int;
                    vmMachine_push(state, newOp);
                } else {
                    printf("%s %s\n",OpCodeTypeStrings[op.type], OpCodeTypeStrings[op1.type]);
                    assert(false);
                }

                break;
            }
            case OP_CODE_MULTIPLY: {
                VmNumberType op = popAndGetValueNumber(state, OP_CODE_NONE);
                VmNumberType op1 = popAndGetValueNumber(state, OP_CODE_NONE);

                VmOperation newOp = {};
                if(op.type == OP_CODE_FLOAT || op1.type == OP_CODE_FLOAT) {
                    newOp.type = OP_CODE_FLOAT;
                    newOp.as_float = vm_getLiteralValueAsFloat(op1) * vm_getLiteralValueAsFloat(op);
                    vmMachine_push(state, newOp);
                } else if(op.type == OP_CODE_UINT && op1.type == OP_CODE_UINT) {
                    newOp.type = OP_CODE_UINT;
                    newOp.as_int = op1.as_int * op.as_int;
                    vmMachine_push(state, newOp);
                } else {
                    printf("%s %s\n",OpCodeTypeStrings[op.type], OpCodeTypeStrings[op1.type]);
                    assert(false);
                }
                break;
            }
            case OP_CODE_DIVIDE: {
                VmNumberType op = popAndGetValueNumber(state, OP_CODE_NONE);
                VmNumberType op1 = popAndGetValueNumber(state, OP_CODE_NONE);
                VmOperation newOp = {};
                if(op.type == OP_CODE_FLOAT || op1.type == OP_CODE_FLOAT) {
                    newOp.type = OP_CODE_FLOAT;
                    newOp.as_float = vm_getLiteralValueAsFloat(op1) / vm_getLiteralValueAsFloat(op);
                    vmMachine_push(state, newOp);
                } else if(op.type == OP_CODE_UINT && op1.type == OP_CODE_UINT) {
                    newOp.type = OP_CODE_UINT;
                    newOp.as_int = op1.as_int / op.as_int;
                    vmMachine_push(state, newOp);
                } else {
                    printf("%s %s\n",OpCodeTypeStrings[op.type], OpCodeTypeStrings[op1.type]);
                    assert(false);
                }
                break;
            }
            case OP_CODE_POWER_TO: {
                VmNumberType op = popAndGetValueNumber(state, OP_CODE_NONE);
                VmNumberType op1 = popAndGetValueNumber(state, OP_CODE_NONE);
                VmOperation newOp = {};
                if(op.type == OP_CODE_FLOAT || op1.type == OP_CODE_FLOAT) {
                    newOp.type = OP_CODE_FLOAT;
                    newOp.as_float = pow(vm_getLiteralValueAsFloat(op1), vm_getLiteralValueAsFloat(op));
                    vmMachine_push(state, newOp);
                } else if(op.type == OP_CODE_UINT && op1.type == OP_CODE_UINT) {
                    newOp.type = OP_CODE_UINT;
                    newOp.as_int = pow(op1.as_int, op.as_int);
                    vmMachine_push(state, newOp);
                } else {
                    printf("%s %s\n",OpCodeTypeStrings[op.type], OpCodeTypeStrings[op1.type]);
                    assert(false);
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
            case OP_CODE_BYTE_OFFSET_REFERENCE: {
                u64 isArray = popAndGetValueNumber(state, OP_CODE_UINT).as_uint;

                u64 addintionalIndex = popAndGetValueNumber(state, OP_CODE_UINT).as_uint;
                //NOTE: Get the byte offset value
                VmOperation offsetOp = vmMachine_pop(state);
                assert(offsetOp.type == OP_CODE_VARIABLE_REFERENCE_OFFSET);
                //NOTE: Just get the reference out, we don't want to push the whole thing to the stack
                u64 offset = offsetOp.as_uint;

                //NOTE: We get the variable out to get the arrayLength. Our Type checker could pick this up if we updated the variable array lengths as we
                //      ran the interpreter when emitting the byte code. We can do this now if the array size was fixed but becuase we allow dynamic types,
                //      the length can just throught the life of the compile. As a way round this now, we just get the current variable state out to check it.

                StackVariable *var = getStackVariable(state, op->name);
                assert(var);
                int arrayLength = var->count;

                if(addintionalIndex >= arrayLength) {
                    assert(!"Array out of bounds error");
                }
                 if(addintionalIndex < 0) {
                    assert(!"Array out of bounds error");
                }

                VmNumberType numberType = getValueFromStack(state, offset + addintionalIndex*sizeof(VmOperation), OP_CODE_NONE);

                VmOperation newOp = {};
                newOp.type = numberType.type;
                newOp.raw = numberType.raw;
                vmMachine_push(state, newOp);

            } break;
             case OP_CODE_BYTE_OFFSET_WRITE: {

                u64 addintionalIndex = popAndGetValueNumber(state, OP_CODE_UINT).as_uint;
                //NOTE: Get the byte offset value
                VmOperation offsetOp = vmMachine_pop(state);
                assert(offsetOp.type == OP_CODE_VARIABLE_REFERENCE_OFFSET);
                //NOTE: Just get the reference out, we don't want to push the whole thing to the stack
                u64 offset = offsetOp.as_uint;

                u64 isArray = popAndGetValueNumber(state, OP_CODE_UINT).as_uint;
                u64 arrayLength = popAndGetValueNumber(state, OP_CODE_UINT).as_uint;
                VmNumberType numberType = popAndGetValueNumber(state, OP_CODE_NONE);

                if(addintionalIndex >= arrayLength) {
                    assert(!"Array out of bounds error");
                }
                if(addintionalIndex < 0) {
                    assert(!"Array out of bounds error");
                }

                setValueFromStack(state, offset + addintionalIndex*sizeof(VmOperation), numberType);
            } break;
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
                int count = op->as_uint;
                //NOTE: Variable reference, so instead of pushing the variable on the stack, we push the offset on
                StackVariable *var = getStackVariable(state, op->name);
                assert(var);
                // result.count = var->count;

                 //NOTE: The length we want to get out
                VmOperation newOp1 = {};
                newOp1.type = OP_CODE_UINT;
                newOp1.as_uint = count;
                vmMachine_push(state, newOp1);

                VmOperation newOp2 = {};
                newOp2.type = OP_CODE_UINT;
                newOp2.as_uint = (var->type == OP_CODE_NUMBER_ARRAY) ? 1 : 0;
                vmMachine_push(state, newOp2);

                VmOperation newOp = {};
                newOp.type = OP_CODE_VARIABLE_REFERENCE_OFFSET;
                newOp.as_uint = var->bytesOffset;
                vmMachine_push(state, newOp);

                break;
            }
            case OP_CODE_RECORD_TYPE: {
                VmOperation toPush = *op;
                at += sizeof(VmOperation);
                sizeToMove = op->as_uint;
                vmMachine_pushData(state, at, op->as_uint);
                vmMachine_push(state, toPush);
            } break;
            case OP_CODE_VARIABLE_ASSIGN: {
                VmNumberType value = popAndGetValueNumber(state, OP_CODE_NONE);

              if(value.count > 1 || value.type == OP_CODE_NUMBER_ARRAY) {
                    VmOperation *tempArray = pushArray(&globalPerFrameArena, value.count, VmOperation);

                    for(int i = 0; i < value.count; ++i) {
                        VmNumberType arrayValue = popAndGetValueNumber(state, OP_CODE_NONE);
                        tempArray[i].type = arrayValue.type;
                        tempArray[i].raw = arrayValue.raw;
                    }
                    pushStackVariable(state, op->name, tempArray, value.count, OP_CODE_NUMBER_ARRAY, sizeof(VmOperation)*value.count);
                } else {
                    u64 structSize = sizeof(VmOperation);
                    if(value.type == OP_CODE_RECORD_TYPE) {
                        structSize = value.as_uint;
                    }
                    VmOperation opcode = { .type = value.type, .raw = value.raw };
                    pushStackVariable(state, op->name, &opcode, 1, value.type, structSize);
                }
                break;
            }

            default: {
                // Error handling here
                break;
            }
        }
        at += sizeToMove;
    }
    return clear;
}
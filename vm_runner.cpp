enum VmErrorCode {
    VM_ERROR_STACK_OVERFLOW,
    VM_ERROR_STACK_UNDERFLOW,
};

struct StackVariable {
    char *name;
    u64 bytesOffset;

    StackVariable *next; //NOTE: Linked list pointer
};

struct VmMachineState {
    u8 *stackBase;
    u8 *at;
    u64 stackSizeBytes;
    u64 stackSizeMaxBytes;
    bool useRadians;

    char *panic; //NOTE: Error

    StackVariable *variables[MAX_VARIABLE_MAP_SIZE]; //NOTE: Nodes pushed onto per frame arena 
};

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

void pushStackVariable(VmMachineState *state, char *name, double value) {
    StackVariable *var = pushStruct(&globalPerFrameArena, StackVariable);

    var->name = name;
    var->bytesOffset = (u64)(state->at - state->stackBase);

    VmOperation newOp = {};
    newOp.type = OP_CODE_VARIABLE_DECLARATION;
    newOp.value_ = value;
    
    vmMachine_push(state, newOp);

    int index = getIndexForVariableMap(name);

    StackVariable **ptr = &state->variables[index];
    while(*ptr) {
        ptr = &(*ptr)->next;
    }
    *ptr = var;
}

StackVariable *getStackVariable(VmMachineState *state, char *name) {
    int index = getIndexForVariableMap(name);

    StackVariable *ptr = state->variables[index];
    assert(ptr);
    bool found = false;
    while(ptr && !found) {
        if(easyString_stringsMatch_nullTerminated(name, ptr->name)) {
            
            found = true;
            break;
        } else {
            ptr = ptr->next;
        }
    }
    assert(found);
    return ptr;
}

double vmOp_getValue(VmMachineState *state, VmOperation op) {
    double result = 0;
    if(op.type == OP_CODE_NUMBER) {
        result = op.value_;
    } else if(op.type == OP_CODE_VARIABLE_REFERENCE) {
        assert(op.name);
        StackVariable *var = getStackVariable(state, op.name);
        assert(var);
        VmOperation *dec = (VmOperation *)(state->stackBase + var->bytesOffset);
        assert(dec->type == OP_CODE_VARIABLE_DECLARATION);
        result = dec->value_;
    } else {
        state->panic = "Expected a number of variable";
        assert(false);
    }
    return result;
}

bool vm_isError(VmOperation op) {
    return(op.type == OP_CODE_ERROR);

}

VmMachineState initVmMachineState() {
    VmMachineState state = {};
    state.stackSizeMaxBytes = Megabytes(5);
    state.at = state.stackBase = (u8 *)pushSize(&globalPerFrameArena, state.stackSizeMaxBytes);
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

double popAndGetValueNumber(VmMachineState *state) {
    double result = 0;
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

bool runCode(GameState *gameState, VmOperation *operations, int operationCount) {
    VmMachineState state = initVmMachineState();

    bool clear = false;

    for(int i = 0; i < operationCount; ++i) {
        VmOperation *op = operations + i;

        printf("%s\n", OpCodeTypeStrings[op->type]);

        switch (op->type) {
            // --- Vector Operations ---
            case OP_CODE_VECTOR_LENGTH: {
                double vectorSize = popAndGetValueNumber(&state);
                VmOperation newOp = {};
                newOp.type = OP_CODE_NUMBER;

                if(vectorSize == 3) {
                    newOp.value_ = float3_magnitude(make_float3(popAndGetValueNumber(&state), popAndGetValueNumber(&state), popAndGetValueNumber(&state)));
                } else if(vectorSize == 2) {
                    newOp.value_ = float2_magnitude(make_float2(popAndGetValueNumber(&state), popAndGetValueNumber(&state)));
                }
                
                vmMachine_push(&state, newOp);
                break;
            }
            case OP_CODE_PRINT: {
                double value = popAndGetValueNumber(&state);

                StringBuffer b = {};
                b.string = easy_createString_printf(&globalPerVmRunLifetime, "%f", value);

                printf("%s\n", b.string);

                // pushArrayItem(&gameState->declarationsInput, b, StringBuffer);
                break;
            }
            case OP_CODE_DOT_PRODUCT: {
                double vectorSize = popAndGetValueNumber(&state);
                VmOperation newOp = {};
                newOp.type = OP_CODE_NUMBER;

                if(vectorSize == 3) {
                    newOp.value_ = float3_dot(make_float3(popAndGetValueNumber(&state), popAndGetValueNumber(&state), popAndGetValueNumber(&state)), make_float3(popAndGetValueNumber(&state), popAndGetValueNumber(&state), popAndGetValueNumber(&state)));
                } else if(vectorSize == 2) {
                    newOp.value_ = float2_dot(make_float2(popAndGetValueNumber(&state), popAndGetValueNumber(&state)), make_float2(popAndGetValueNumber(&state), popAndGetValueNumber(&state)));
                }
                
                vmMachine_push(&state, newOp);
                break;
            }
            case OP_CODE_CROSS_PRODUCT: {
                 double vectorSize = popAndGetValueNumber(&state);
                

                if(vectorSize == 3) {
                    float3 vecRes = float3_cross(make_float3(popAndGetValueNumber(&state), popAndGetValueNumber(&state), popAndGetValueNumber(&state)), make_float3(popAndGetValueNumber(&state), popAndGetValueNumber(&state), popAndGetValueNumber(&state)));
                    vmMachine_push(&state, {.type = OP_CODE_NUMBER, .value_ = vecRes.z});
                    vmMachine_push(&state, {.type = OP_CODE_NUMBER, .value_ = vecRes.y});
                    vmMachine_push(&state, {.type = OP_CODE_NUMBER, .value_ = vecRes.x});
                } else if(vectorSize == 2) {
                    float vecRes = float2_cross(make_float2(popAndGetValueNumber(&state), popAndGetValueNumber(&state)), make_float2(popAndGetValueNumber(&state), popAndGetValueNumber(&state)));
                    vmMachine_push(&state, {.type = OP_CODE_NUMBER, .value_ = vecRes});
                }
                
                break;
            }

            // --- Trigonometry ---
            case OP_CODE_SIN: {
                double value = popAndGetValueNumber(&state);
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = sin(vm_getAngle(&state, value));
                
                vmMachine_push(&state, newOp);
                break;
            }
            case OP_CODE_COS: {
                double value = popAndGetValueNumber(&state);
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = cos(vm_getAngle(&state, value));
                vmMachine_push(&state, newOp);
                break;
            }
            case OP_CODE_TAN: {
                double value = popAndGetValueNumber(&state);
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = tan(vm_getAngle(&state, value));
                vmMachine_push(&state, newOp);
                break;
            }
            case OP_CODE_ARCSIN: {
                double value = popAndGetValueNumber(&state);
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = asin(vm_getAngle(&state, value));
                vmMachine_push(&state, newOp);
                break;
            }
            case OP_CODE_ARCCOS: {
                double value = popAndGetValueNumber(&state);
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = acos(vm_getAngle(&state, value));
                vmMachine_push(&state, newOp);
                break;
            }
            case OP_CODE_ARCTAN: {
                double value = popAndGetValueNumber(&state);
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = atan(vm_getAngle(&state, value));
                vmMachine_push(&state, newOp);
                break;
            }
            case OP_CODE_ARCTAN2: {
                double value = popAndGetValueNumber(&state);
                double value1 = popAndGetValueNumber(&state);
                VmOperation newOp = { .type = OP_CODE_NUMBER};
                newOp.value_ = atan2(vm_getAngle(&state, value1), vm_getAngle(&state, value));
                vmMachine_push(&state, newOp);
                break;
            }

            // --- Configuration ---
            case OP_CODE_SET_DEGREES_MODE: {
                state.useRadians = false;
                break;
            }
            case OP_CODE_SET_RADIANS_MODE: {
                state.useRadians = true;
                break;
            }

            // --- Arithmetic ---
            case OP_CODE_ADD: {
                VmOperation op = vmMachine_pop(&state);
                VmOperation op1 = vmMachine_pop(&state);
                if(!vm_isError(op) && !vm_isError(op1)) {
                    VmOperation newOp = {};
                    newOp.type = OP_CODE_NUMBER;
                    newOp.value_ = vmOp_getValue(&state, op) + vmOp_getValue(&state, op1);
                    vmMachine_push(&state, newOp);
                    printf("%f\n", newOp.value_);
                    
                }   
                break;
            }
            case OP_CODE_MINUS: {
                VmOperation op = vmMachine_pop(&state);
                VmOperation op1 = vmMachine_pop(&state);
                if(!vm_isError(op) && !vm_isError(op1)) {
                    VmOperation newOp = {};
                    newOp.type = OP_CODE_NUMBER;
                    newOp.value_ = vmOp_getValue(&state, op1) - vmOp_getValue(&state, op);
                    vmMachine_push(&state, newOp);
                    printf("%f\n", newOp.value_);
                }   
                break;
            }
            case OP_CODE_MULTIPLY: {
                 VmOperation op = vmMachine_pop(&state);
                VmOperation op1 = vmMachine_pop(&state);
                if(!vm_isError(op) && !vm_isError(op1)) {
                    VmOperation newOp = {};
                    newOp.type = OP_CODE_NUMBER;
                    newOp.value_ = vmOp_getValue(&state, op) * vmOp_getValue(&state, op1);
                    vmMachine_push(&state, newOp);
                    printf("%f\n", newOp.value_);
                }   
                break;
            }
            case OP_CODE_DIVIDE: {
                 VmOperation op = vmMachine_pop(&state);
                VmOperation op1 = vmMachine_pop(&state);
                if(!vm_isError(op) && !vm_isError(op1)) {
                    VmOperation newOp = {};
                    newOp.type = OP_CODE_NUMBER;
                    newOp.value_ = vmOp_getValue(&state, op1) / vmOp_getValue(&state, op);
                    vmMachine_push(&state, newOp);
                    printf("%f\n", newOp.value_);
                }   
                break;
            }
              case OP_CODE_POWER_TO: {
                VmOperation op = vmMachine_pop(&state);
                VmOperation op1 = vmMachine_pop(&state);
                if(!vm_isError(op) && !vm_isError(op1)) {
                    VmOperation newOp = {};
                    newOp.type = OP_CODE_NUMBER;
                    newOp.value_ = pow(vmOp_getValue(&state, op1), vmOp_getValue(&state, op));
                    vmMachine_push(&state, newOp);
                    printf("%f\n", newOp.value_);
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
                vmMachine_push(&state, *op);
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
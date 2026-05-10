#define EASY_OP_CODE_TYPE(FUNC) \
    FUNC(OP_CODE_NONE) \
    FUNC(OP_CODE_VECTOR_LENGTH) \
    FUNC(OP_CODE_VECTOR_NORMALIZE) \
    FUNC(OP_CODE_DOT_PRODUCT) \
    FUNC(OP_CODE_CROSS_PRODUCT) \
    FUNC(OP_CODE_QUADRATIC) \
    FUNC(OP_CODE_SIN) \
    FUNC(OP_CODE_COS) \
    FUNC(OP_CODE_SQR) \
    FUNC(OP_CODE_SQRT) \
    FUNC(OP_CODE_TAN) \
    FUNC(OP_CODE_ARCSIN) \
    FUNC(OP_CODE_ARCCOS) \
    FUNC(OP_CODE_ARCTAN) \
    FUNC(OP_CODE_ARCTAN2) \
    FUNC(OP_CODE_SET_DEGREES_MODE) \
    FUNC(OP_CODE_SET_RADIANS_MODE) \
    FUNC(OP_CODE_PRINT) \
    FUNC(OP_CODE_ADD) \
    FUNC(OP_CODE_MINUS) \
    FUNC(OP_CODE_NEGATE) \
    FUNC(OP_CODE_MULTIPLY) \
    FUNC(OP_CODE_POWER_TO) \
    FUNC(OP_CODE_DIVIDE) \
    FUNC(OP_CODE_BIT_SHIFT_LEFT) \
    FUNC(OP_CODE_BIT_SHIFT_RIGHT) \
    FUNC(OP_CODE_BIT_OP_AND) \
    FUNC(OP_CODE_BIT_OP_OR) \
    FUNC(OP_CODE_FLOAT) \
    FUNC(OP_CODE_UINT) \
    FUNC(OP_CODE_NUMBER_ARRAY) \
    FUNC(OP_CODE_STRING) \
    FUNC(OP_CODE_VARIABLE_ASSIGN) \
    FUNC(OP_CODE_VARIABLE_TYPE) \
    FUNC(OP_CODE_VARIABLE_REFERENCE) \
    FUNC(OP_CODE_SUMMATION) \
    FUNC(OP_CODE_DECLARE) \
    FUNC(OP_CODE_CLEAR) \
    FUNC(OP_CODE_PRINT_AS_BINARY) \
    FUNC(OP_CODE_PRINT_AS_COLOR) \
    FUNC(OP_CODE_PRINT_AS_HEXADECIMAL) \
    FUNC(OP_CODE_ERROR) \
    FUNC(OP_CODE_RECORD_TYPE) \
    FUNC(OP_CODE_BYTE_OFFSET_REFERENCE) \
    FUNC(OP_CODE_BYTE_OFFSET_WRITE) \
    FUNC(OP_CODE_ARRAY_ACCESSOR) \
    FUNC(OP_CODE_VARIABLE_REFERENCE_OFFSET) \

typedef enum {
    EASY_OP_CODE_TYPE(ENUM)
} OpCode;

static char *OpCodeTypeStrings[] = {
    EASY_OP_CODE_TYPE(STRING)
};

enum VmErrorCode {
    VM_ERROR_STACK_OVERFLOW,
    VM_ERROR_STACK_UNDERFLOW,
};

struct StackVariable {
    char *name;
    u64 bytesOffset;
    int count; //NOTE: If more than 1 it's an array variable
    OpCode type; //NOTE: could be a Number array of size 1, so we need this aswell
    u64 structSize; //NOTE: The size of the object

    StackVariable *next; //NOTE: Linked list pointer
};

//NOTE: If you want to print the value in a certain format
enum PrintFormatType {
    VM_PRINT_FORMAT_DEFAULT,
    VM_PRINT_FORMAT_BINARY,
    VM_PRINT_FORMAT_HEX,
    VM_PRINT_FORMAT_COLOR,
};

struct VmOperation {
    OpCode type;
    PrintFormatType printFormat;
    union {
        int64_t as_int;
        uint64_t as_uint;
        double  as_float;
        uint64_t raw;
    };
    char *name; //NOTE: For variables allocated on the per vm run arena
};

struct VmNumberType {
    OpCode type;
    PrintFormatType printFormat;
    union {
        int64_t as_int;
        uint64_t as_uint;
        double  as_float;
        uint64_t raw;

    };
    char *name; //NOTE: If it's a string value

    int count; //NOTE: If more than 1 it's an array type and there still on the stack
};

#define MAX_VARIABLE_MAP_SIZE 4096
struct VmMachineState {
    u8 *stackBase;
    u8 *at;
    u64 stackSizeBytes;
    u64 stackSizeMaxBytes;
    bool useRadians;

    char *panic; //NOTE: Error

    StackVariable *variables[MAX_VARIABLE_MAP_SIZE]; //NOTE: Nodes pushed onto per frame arena
};

int getIndexForVariableMap(char *name) {
    u32 hash = get_crc32(name, easyString_getSizeInBytes_utf8(name));
    int index = hash % MAX_VARIABLE_MAP_SIZE;
    return index;
}
enum OpCode {
    OP_CODE_NONE,

    OP_CODE_VECTOR_LENGTH,
    OP_CODE_DOT_PRODUCT,
    OP_CODE_CROSS_PRODUCT,
    OP_CODE_SIN,
    OP_CODE_COS,
    OP_CODE_TAN,
    OP_CODE_ARCSIN,
    OP_CODE_ARCCOS,
    OP_CODE_ARCTAN,
    OP_CODE_ARCTAN2,
    OP_CODE_SET_DEGREES_MODE,
    OP_CODE_SET_RADIANS_MODE,
    OP_CODE_PRINT,

    OP_CODE_ADD,
    OP_CODE_MINUS,
    OP_CODE_MULTIPLY,
    OP_CODE_DIVIDE,

    //NOTE: Values
    OP_CODE_NUMBER,
    OP_CODE_STRING,
    OP_CODE_VARIABLE_DECLARATION,
    OP_CODE_VARIABLE_ASSIGN,
    OP_CODE_VARIABLE_REFERENCE,

    OP_CODE_DECLARE,

    OP_CODE_ERROR,
};

struct VmOperation {
    OpCode type;
    double value_;
    char *name; //NOTE: For variables allocated on the per vm run arena
};

#define MAX_VARIABLE_MAP_SIZE 4096
int getIndexForVariableMap(char *name) {
    u32 hash = get_crc32(name, easyString_getSizeInBytes_utf8(name));
    int index = hash % MAX_VARIABLE_MAP_SIZE;
    return index;
}
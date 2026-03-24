#define EASY_OP_CODE_TYPE(FUNC) \
    FUNC(OP_CODE_NONE) \
    FUNC(OP_CODE_VECTOR_LENGTH) \
    FUNC(OP_CODE_DOT_PRODUCT) \
    FUNC(OP_CODE_CROSS_PRODUCT) \
    FUNC(OP_CODE_SIN) \
    FUNC(OP_CODE_COS) \
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
    FUNC(OP_CODE_MULTIPLY) \
    FUNC(OP_CODE_POWER_TO) \
    FUNC(OP_CODE_DIVIDE) \
    FUNC(OP_CODE_NUMBER) \
    FUNC(OP_CODE_STRING) \
    FUNC(OP_CODE_VARIABLE_ASSIGN) \
    FUNC(OP_CODE_VARIABLE_REFERENCE) \
    FUNC(OP_CODE_DECLARE) \
    FUNC(OP_CODE_CLEAR) \
    FUNC(OP_CODE_ERROR)

typedef enum {
    EASY_OP_CODE_TYPE(ENUM)
} OpCode;

static char *OpCodeTypeStrings[] = { 
    EASY_OP_CODE_TYPE(STRING) 
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
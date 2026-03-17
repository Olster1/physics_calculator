struct StringBuffer {
    char *string;
    int cursor;
};

void stringBuffer_concatString(StringBuffer *buffer, char *str) {
    if(!str) {
        return;
    }

    char *newString = easy_createString_printf(&globalPerFrameArena, "%s%s", (buffer->string) ? buffer->string : "", str);

    if(buffer->string) {
       easyPlatform_freeMemory(buffer->string); 
    }

    u64 strSizeInBytes = easyString_getSizeInBytes_utf8(newString);
    buffer->string = nullTerminate(newString, strSizeInBytes);
    buffer->cursor = strSizeInBytes;
}

void stringBuffer_popCharacter(StringBuffer *buffer) {
    if(buffer->string && buffer->cursor > 0) {
       buffer->string[--buffer->cursor] = 0;
    }
}

void stringBuffer_clear(StringBuffer *buffer) {
    if(buffer->string) {
       easyPlatform_freeMemory(buffer->string); 
       buffer->string = 0;
    }
}
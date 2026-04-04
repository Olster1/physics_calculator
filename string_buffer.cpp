struct StringBuffer {
    char *string;
    int cursor;
};

void stringBuffer_cursorRight(StringBuffer *buffer, int count) {
    if(buffer->string) {
        int maxLength = easyString_getStringLength_utf8(buffer->string);
        for(int i = 0; i < count; i++) {
            if(buffer->cursor < (maxLength)) {
                buffer->cursor++;
            }
        }
    }
}

void stringBuffer_cursorLeft(StringBuffer *buffer, int count) {
    if(buffer->string) {
        for(int i = 0; i < count; i++) {
            if(buffer->cursor > 0) {
            buffer->cursor--;
            }    
        }
    }
}
void stringBuffer_insertString(StringBuffer *buffer, char *str) {
    if (!str) {
        return;
    }

    // 1. Handle the case where the buffer is currently empty
    if (!buffer->string) {
        assert(false);
        //NOTE: Should have been initialized;
        return;
    }

    // 2. Split the existing string at cursorAt and insert 'str'
    // We use "%.*s" to print only up to 'cursorAt' characters for the prefix
    char *newString = easy_createString_printf(
        &globalPerFrameArena, 
        "%.*s%s%s", 
        (int)buffer->cursor, buffer->string, // Prefix: up to cursorAt
        str,                                   // Inserted string
        buffer->string + buffer->cursor      // Suffix: from cursorAt to end
    );

    // 3. Clean up the old buffer
    easyPlatform_freeMemory(buffer->string); 

    // 4. Update buffer state
    u64 addedLen = easyString_getSizeInBytes_utf8(str);
    u64 newTotalSize = easyString_getSizeInBytes_utf8(newString);
    
    buffer->string = nullTerminate(newString, newTotalSize);
    
    // Move the cursor to the end of the newly inserted text
    buffer->cursor += addedLen; 
}
void clearStringBuffer(StringBuffer *buffer) {
    if(buffer->string) {
       easyPlatform_freeMemory(buffer->string); 
    }
    buffer->cursor = 0;
    buffer->string = 0;

    buffer->string = nullTerminate(0, 0);
}

void stringBuffer_init(StringBuffer *buffer) {
    buffer->string = 0;
    buffer->cursor = 0;

    buffer->string = nullTerminate(0, 0);
}

void stringBuffer_removeCharacter(StringBuffer *buffer) {
    if(buffer->string && buffer->cursor > 0) {
        int maxLength = easyString_getStringLength_utf8(buffer->string);
        for(int i = buffer->cursor; i <= maxLength; ++i) {
            buffer->string[i - 1] = buffer->string[i];
        }
        buffer->cursor--;
        assert(buffer->cursor >= 0);
    }
    
}

void stringBuffer_clear(StringBuffer *buffer) {
    if(buffer->string) {
       easyPlatform_freeMemory(buffer->string); 
       buffer->string = 0;
    }
}
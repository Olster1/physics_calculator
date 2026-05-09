char *save_getSettingsFileName(Arena *arena) {
    char *filePath = getPlatformSaveFilePath();
    char *strToWrite = easy_createString_printf(arena, "%s%s", (char *)filePath, "game.settings");
    free(filePath);
    return strToWrite;
}

char *getInbuiltStructCode() {
    // return "";//"struct Math { pi = 3.14159265358979\n g = 9.807\n }\n math = Math() \n";
    return "pi = 3.14159265358979\n g = 9.807\nhalf_g = 4.9035\ne = 2.71828182845904\nphi = 1.61803398874989\nc = 299792458\n";
}

void saveSettingsFile(SettingsToSave *settings) {
    char *filePath = save_getSettingsFileName(&globalPerFrameArena);
    game_file_handle json = platformBeginFileWrite((char *)filePath);
    assert(!json.HasErrors);
    size_t offset = 0;

    char *strToWrite = "";
    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"windowSize\": %d %d}\n", settings->windowX, settings->windowY);
    offset = platformWriteFile(&json, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"windowPos\": %d %d}\n", settings->windowPosX, settings->windowPosY);
    offset = platformWriteFile(&json, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"themeIndex\": %d}\n", settings->themeIndex);
    offset = platformWriteFile(&json, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"useRadians\": %d}\n", settings->useRadians);
    offset = platformWriteFile(&json, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"startUseRadians\": %d}\n", settings->startUseRadians);
    offset = platformWriteFile(&json, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"code\": \"%s\"}\n", settings->code);
    offset = platformWriteFile(&json, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"codeCheckSum\": %u}\n", get_crc32(settings->code, easyString_getSizeInBytes_utf8(settings->code)));
    offset = platformWriteFile(&json, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    platformEndFile(json);
}

struct LoadSettingsFileResult {
    SettingsToSave settingsToSave;
};

LoadSettingsFileResult loadSettingsFile(SettingsToSave *settingsToSave) {
    LoadSettingsFileResult result = {};
    result.settingsToSave.code = getInbuiltStructCode();
    result.settingsToSave.windowX = global_default_window_size_x;
    result.settingsToSave.windowY = global_default_window_size_y;
    return result;
    char *filePath = save_getSettingsFileName(&globalPerFrameArena);
    if(platformDoesFileExist(filePath)) {
        FileContents contents = platformReadEntireFile(&globalPerFrameArena, (char *)filePath, true);
        assert(contents.valid);
        assert(contents.fileSize > 0);
        assert(contents.memory);

        EasyTokenizer tokenizer = lexBeginParsing(contents.memory, EASY_LEX_OPTION_EAT_WHITE_SPACE);
        bool gotStartRadians = false;
        bool gotCode = false;
        bool parsing = true;
        u32 checkSum = 0;
        while(parsing) {
            EasyToken t = lexGetNextToken(&tokenizer);

            if(t.type == TOKEN_NULL_TERMINATOR) {
                parsing = false;
            } else if(t.type == TOKEN_OPEN_BRACKET) {
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_STRING);
                 if(easyString_stringsMatch_null_and_count("windowSize", t.at, t.size)) {
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_COLON);
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_INTEGER);
                    if(t.type == TOKEN_INTEGER) {
                        result.settingsToSave.windowX = t.intVal;
                    }
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_INTEGER);
                    if(t.type == TOKEN_INTEGER) {
                        result.settingsToSave.windowY = t.intVal;
                    }
                } else if(easyString_stringsMatch_null_and_count("themeIndex", t.at, t.size)) {
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_COLON);
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_INTEGER);
                    if(t.type == TOKEN_INTEGER) {
                        result.settingsToSave.themeIndex = t.intVal;
                    }
                } else if(easyString_stringsMatch_null_and_count("code", t.at, t.size)) {

                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_COLON);
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_STRING);
                    if(t.type == TOKEN_STRING) {
                        gotCode = true;
                        result.settingsToSave.code = nullTerminateArena(t.at, t.size, &globalPerClearSessionArena);
                    }
                 } else if(easyString_stringsMatch_null_and_count("codeCheckSum", t.at, t.size)) {
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_COLON);
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_INTEGER);
                    if(t.type == TOKEN_INTEGER) {
                        checkSum = (u32)t.intVal;
                    }
                } else if(easyString_stringsMatch_null_and_count("useRadians", t.at, t.size)) {
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_COLON);
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_INTEGER);
                    if(t.type == TOKEN_INTEGER) {
                        result.settingsToSave.useRadians = t.intVal;
                    }
                } else if(easyString_stringsMatch_null_and_count("startUseRadians", t.at, t.size)) {
                    gotStartRadians = true;
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_COLON);
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_INTEGER);
                    if(t.type == TOKEN_INTEGER) {
                        result.settingsToSave.startUseRadians = t.intVal;
                    }
                } else if(easyString_stringsMatch_null_and_count("windowPos", t.at, t.size)) {
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_COLON);
                    t = lexGetNextToken(&tokenizer);
                    // if(t.type == TOKEN_INTEGER);
                    if(t.type == TOKEN_INTEGER) {
                        result.settingsToSave.windowPosX = t.intVal;
                    }
                    t = lexGetNextToken(&tokenizer);
                    // assert(t.type == TOKEN_INTEGER);
                    if(t.type == TOKEN_INTEGER) {
                        result.settingsToSave.windowPosY = t.intVal;
                    }
                 }
            }
        }

        if(gotCode) {
            u32 testCheckSum = get_crc32(result.settingsToSave.code, easyString_getSizeInBytes_utf8(result.settingsToSave.code));
            if(testCheckSum != checkSum) {
                //NOTE: Don't set the code
                gotCode = false;
                result.settingsToSave.code = getInbuiltStructCode();

            }
        }

        if(!gotStartRadians || !gotCode) {
            //NOTE: Make sure they're correct otherwise it will go out of sync
            result.settingsToSave.startUseRadians = result.settingsToSave.useRadians;
        }
    }
    return result;
}
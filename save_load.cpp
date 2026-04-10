char *save_getSettingsFileName(Arena *arena) {
    char *filePath = getPlatformSaveFilePath();
    char *strToWrite = easy_createString_printf(arena, "%s%s", (char *)filePath, "game.settings");
    free(filePath);
    return strToWrite;
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

    platformEndFile(json);
}

struct LoadSettingsFileResult {
    SettingsToSave settingsToSave;
};

LoadSettingsFileResult loadSettingsFile(SettingsToSave *settingsToSave) {
    LoadSettingsFileResult result = {};
    result.settingsToSave.windowX = global_default_window_size_x;
    result.settingsToSave.windowY = global_default_window_size_y;
    char *filePath = save_getSettingsFileName(&globalPerFrameArena);
    if(platformDoesFileExist(filePath)) {
        FileContents contents = platformReadEntireFile(&globalPerFrameArena, (char *)filePath, true);
        assert(contents.valid);
        assert(contents.fileSize > 0);
        assert(contents.memory);

        EasyTokenizer tokenizer = lexBeginParsing(contents.memory, EASY_LEX_OPTION_EAT_WHITE_SPACE);

        bool parsing = true;
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
    }
    return result;
}
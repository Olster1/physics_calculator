#define BOARD_SAVE_STATE_VERSION 1
#pragma pack(push, 1) 
struct BoardStateFile {
    u64 version;     // u64 is good
    u32 boardSize;    // Explicit 32-bit

    u32 colorId;
    float zoomFactor;     // 4 bytes, safe
    float2 cameraPos;     // 4 bytes, safe

    u64 idOffset;    // Changed from size_t to fixed 64-bit
    u64 idBytesLength;

    u64 boardValuesOffset;
    u64 targetValuesOffset;
    u64 mathGroupIdOffset;

    u32 mathGroupCount;
    
    u64 mathGroupTypesOffset;
    u64 mathGroupAnswersOffset;
    u64 mathGroupSizesOffset;
    u64 mathGroupPositionsOffset;
    u64 mathGroupFarthestPositionOffset;
    u64 mathGroupIdsOffset;
    u64 remainingCellCoordsOffset;

    u32 remainingCellCoordsCount;
};
#pragma pack(pop)

char *save_getSettingsFileName(Arena *arena) {
    char *filePath = getPlatformSaveFilePath();
    char *strToWrite = easy_createString_printf(arena, "%s%s", (char *)filePath, "game.settings");
    free(filePath);
    return strToWrite;
}

char *save_getBoardFileName(char *id, Arena *arena) {
    char *filePath = getPlatformSaveFilePath();
    char *strToWrite = easy_createString_printf(arena, "%s%s", (char *)filePath, id);
    free(filePath);
    return strToWrite;
}

char *loadGame(BoardState *state, char *idToLoad, SoundAssets *soundAssets, SettingsToSave *settingsToSave);

void saveSettingsFile(SettingsToSave *settings, char *playingFileId) {
    char *filePath = save_getSettingsFileName(&globalPerFrameArena);
    game_file_handle json = platformBeginFileWrite((char *)filePath);
    assert(!json.HasErrors);
    size_t offset = 0;

    settings->playingFileId = playingFileId;
    
    char *strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"playingFile\": \"%s\"}\n", (playingFileId) ? playingFileId : "");
    offset = platformWriteFile(&json, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"windowSize\": %d %d}\n", settings->windowX, settings->windowY);
    offset = platformWriteFile(&json, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"windowPos\": %d %d}\n", settings->windowPosX, settings->windowPosY);
    offset = platformWriteFile(&json, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"volumeBg\": %f}\n", settings->volumeBg);
    offset = platformWriteFile(&json, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    strToWrite = easy_createString_printf(&globalPerFrameArena, "{\"volumeFg\": %f}\n", settings->volumeFg);
    offset = platformWriteFile(&json, strToWrite, easyString_getSizeInBytes_utf8(strToWrite), offset);

    platformEndFile(json);
}

struct LoadSettingsFileResult {
    BoardState state;
    SettingsToSave settingsToSave;
    char *playingFileId;
};

LoadSettingsFileResult loadSettingsFile(SoundAssets *soundAssets, SettingsToSave *settingsToSave) {
    LoadSettingsFileResult result = {};
    result.settingsToSave.windowX = global_default_window_size_x;
    result.settingsToSave.windowY = global_default_window_size_y;
    result.settingsToSave.volumeBg = 1;
    result.settingsToSave.volumeFg = 1;
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
                if(easyString_stringsMatch_null_and_count("playingFile", t.at, t.size)) {
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_COLON);
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_STRING);
                    if(t.type == TOKEN_STRING && t.size > 0) {
                        result.playingFileId = loadGame(&result.state, nullTerminateArena(t.at, t.size, &globalPerFrameArena), soundAssets, settingsToSave);
                        result.settingsToSave.playingFileId = result.playingFileId;
                    }
                } else if(easyString_stringsMatch_null_and_count("windowSize", t.at, t.size)) {
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
                    assert(t.type == TOKEN_INTEGER);
                    if(t.type == TOKEN_INTEGER) {
                        result.settingsToSave.windowPosX = t.intVal;
                    }
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_INTEGER);
                    if(t.type == TOKEN_INTEGER) {
                        result.settingsToSave.windowPosY = t.intVal;
                    }
                 } else if(easyString_stringsMatch_null_and_count("volumeBg", t.at, t.size)) {
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_COLON);
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_FLOAT);
                    if(t.type == TOKEN_FLOAT) {
                        result.settingsToSave.volumeBg = t.floatVal;
                    }
                 } else if(easyString_stringsMatch_null_and_count("volumeFg", t.at, t.size)) {
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_COLON);
                    t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_FLOAT);
                    if(t.type == TOKEN_FLOAT) {
                        result.settingsToSave.volumeFg = t.floatVal;
                    }
                }
            }
        }
    }
    return result;
}

void saveGame(BoardState *state, SettingsToSave *settingsToSave) {
    char *filePath = save_getBoardFileName(state->id.stringID, &globalPerFrameArena);

    game_file_handle file = platformBeginFileWrite((char *)filePath);
    if(!file.HasErrors) {

        BoardStateFile data = {};

        data.version = BOARD_SAVE_STATE_VERSION;
        data.boardSize = state->boardSize;
        data.colorId = state->colorOffset;
        data.zoomFactor = state->zoomFactor;
        data.cameraPos = state->cameraPos;
        data.mathGroupCount = state->mathGroups.typeCount;
        data.remainingCellCoordsCount = state->mathGroups.remainingCellCoordsCount;

        size_t offsetAt = sizeof(BoardStateFile);

        {
            int size = easyString_getSizeInBytes_utf8(state->id.stringID);
            data.idOffset = offsetAt;
            data.idBytesLength = size;
            platformWriteFile(&file, state->id.stringID, size, offsetAt);
            offsetAt += size;
        }

        //NOTE: Allocate the arrays we're going to write
        int boardArea = state->boardSize*state->boardSize;
        int *values = pushArray(&globalPerFrameArena, boardArea, int);
        int *targetValues = pushArray(&globalPerFrameArena, boardArea, int);
        int *mathGroupIds = pushArray(&globalPerFrameArena, boardArea, int);

        int mathGroupTypeCount = state->mathGroups.typeCount;
        int *mathGroupTypes = pushArray(&globalPerFrameArena, mathGroupTypeCount, int);
        int *mathGroupAnswers = pushArray(&globalPerFrameArena, mathGroupTypeCount, int);
        int *mathGroupSizes = pushArray(&globalPerFrameArena, mathGroupTypeCount, int);

        int mathGroupMaxSize = arrayCount(state->mathGroups.mathTypes[0].positions);
        float2 *mathGroupPositions = pushArray(&globalPerFrameArena, mathGroupMaxSize*mathGroupTypeCount, float2);
        float2 *mathGroupFarthestPosition = pushArray(&globalPerFrameArena, mathGroupTypeCount, float2);

        for(int i = 0; i < boardArea; i++) {
            values[i] = state->boardValues[i].value;
            targetValues[i] = state->boardValues[i].targetValue;
            mathGroupIds[i] = state->boardValues[i].mathGroupId;
        }
        
        for(int i = 0; i < state->mathGroups.typeCount; i++) {
            mathGroupTypes[i] = state->mathGroups.mathTypes[i].type;
            mathGroupAnswers[i] = state->mathGroups.mathTypes[i].answer;
            mathGroupSizes[i] = state->mathGroups.mathTypes[i].size;
            mathGroupFarthestPosition[i] = state->mathGroups.mathTypes[i].farthestPosition;

            for(int j = 0; j < mathGroupMaxSize; j++) {
                mathGroupPositions[i*mathGroupMaxSize + j] =  state->mathGroups.mathTypes[i].positions[j];
            }
        }

        {
            size_t size = state->mathGroups.typeCount*sizeof(int);
            data.mathGroupTypesOffset = offsetAt;
            platformWriteFile(&file, mathGroupTypes, size, offsetAt);
            offsetAt += size;

            data.mathGroupAnswersOffset = offsetAt;
            platformWriteFile(&file, mathGroupAnswers, size, offsetAt);
            offsetAt += size;

            data.mathGroupSizesOffset = offsetAt;
            platformWriteFile(&file, mathGroupSizes, size, offsetAt);
            offsetAt += size;

            data.mathGroupFarthestPositionOffset = offsetAt;
            platformWriteFile(&file, mathGroupFarthestPosition, state->mathGroups.typeCount*sizeof(float2), offsetAt);
            offsetAt += state->mathGroups.typeCount*sizeof(float2);

            data.mathGroupPositionsOffset = offsetAt;
            platformWriteFile(&file, mathGroupPositions, state->mathGroups.typeCount*sizeof(float2)*mathGroupMaxSize, offsetAt);
            offsetAt += state->mathGroups.typeCount*sizeof(float2)*mathGroupMaxSize;

        }

        {
            size_t size = boardArea*sizeof(int);
            data.boardValuesOffset = offsetAt;
            platformWriteFile(&file, values, size, offsetAt);
            offsetAt += size;

            data.targetValuesOffset = offsetAt;
            platformWriteFile(&file, targetValues, size, offsetAt);
            offsetAt += size;

            data.mathGroupIdOffset = offsetAt;
            platformWriteFile(&file, mathGroupIds, size, offsetAt);
            offsetAt += size;

            data.remainingCellCoordsOffset = offsetAt;
            platformWriteFile(&file, state->mathGroups.remainingCellCoords, size, offsetAt);
            offsetAt += size;

            data.mathGroupIdsOffset = offsetAt;
            platformWriteFile(&file, state->mathGroups.mathGroupIds, size, offsetAt);
            offsetAt += size;
        }

        platformWriteFile(&file, &data, sizeof(BoardStateFile), 0);

        platformEndFile(file);

        saveSettingsFile(settingsToSave, state->id.stringID);
    } else {
        assert(false);
    }
}

BoardState initBoardState(int boardSize, SoundAssets *soundAssets, SettingsToSave *settingsToSave, bool *avoidMathSymbols = 0, bool generateBoard = true);
char *loadGame(BoardState *state, char *idToLoad, SoundAssets *soundAssets, SettingsToSave *settingsToSave) {
    char *valid = 0;
    char *filePath = save_getBoardFileName(idToLoad, &globalPerFrameArena);
    
    if(platformDoesFileExist(filePath)) {

        FileContents file = platformReadEntireFile(&globalPerFrameArena, (char *)filePath, false);
        assert(file.valid);
        if(file.fileSize >= sizeof(BoardStateFile)) {
            BoardStateFile *data = (BoardStateFile *)file.memory;
            if(data->version == 1) {
                *state = initBoardState(data->boardSize, soundAssets, settingsToSave, 0, false);
                // state->zoomFactor = data->zoomFactor;
                state->cameraPos = data->cameraPos;
                
                state->colorOffset = data->colorId;
                state->mathGroups.typeCount = data->mathGroupCount;
                state->mathGroups.remainingCellCoordsCount = data->remainingCellCoordsCount;

                u8 *fileState = (u8 *)file.memory;

                state->id.stringID = nullTerminateArena((char *)(fileState + data->idOffset), data->idBytesLength, &globalBoardValuesArena);
                state->id.crc32Hash = get_crc32(state->id.stringID, data->idBytesLength);


                // --- Board Values & Target Values ---
                int boardArea = state->boardSize * state->boardSize;
                size_t boardValuesSize = boardArea * sizeof(int);

                int *fileBoardValues = (int *)(fileState + data->boardValuesOffset);
                int *fileTargetValues = (int *)(fileState + data->targetValuesOffset);
                int *fileMathGroupIdPerCell = (int *)(fileState + data->mathGroupIdOffset);

                for(int i = 0; i < boardArea; i++) {
                    state->boardValues[i].value = fileBoardValues[i];
                    state->boardValues[i].targetValue = fileTargetValues[i];
                    state->boardValues[i].mathGroupId = fileMathGroupIdPerCell[i];
                }

                // --- Math Group Metadata ---
                int groupCount = data->mathGroupCount;
                int *fileGroupTypes = (int *)(fileState + data->mathGroupTypesOffset);
                int *fileGroupAnswers = (int *)(fileState + data->mathGroupAnswersOffset);
                int *fileGroupSizes = (int *)(fileState + data->mathGroupSizesOffset);
                float2 *fileGroupFarthest = (float2 *)(fileState + data->mathGroupFarthestPositionOffset);

                for(int i = 0; i < groupCount; i++) {
                    state->mathGroups.mathTypes[i].type = (BoardMathType)fileGroupTypes[i];
                    state->mathGroups.mathTypes[i].answer = fileGroupAnswers[i];
                    state->mathGroups.mathTypes[i].size = fileGroupSizes[i];
                    state->mathGroups.mathTypes[i].farthestPosition = fileGroupFarthest[i];
                }

                // --- Math Group Positions (Flattened) ---
                // Note: We use the same constant/arrayCount logic used during the save
                int mathGroupMaxSize = arrayCount(state->mathGroups.mathTypes[0].positions);
                float2 *filePositions = (float2 *)(fileState + data->mathGroupPositionsOffset);

                for(int i = 0; i < groupCount; i++) {
                    for(int j = 0; j < mathGroupMaxSize; j++) {
                        state->mathGroups.mathTypes[i].positions[j] = filePositions[i * mathGroupMaxSize + j];
                    }
                }

                // --- Miscellaneous Board Arrays ---
                // These are usually simple memcpys since the destination is a flat array
                void *fileMathGroupIds = (void *)(fileState + data->mathGroupIdsOffset);
                easyPlatform_copyMemory(state->mathGroups.mathGroupIds, fileMathGroupIds, boardValuesSize);

                void *fileRemainingCoords = (void *)(fileState + data->remainingCellCoordsOffset);
                easyPlatform_copyMemory(state->mathGroups.remainingCellCoords, fileRemainingCoords, boardValuesSize);

                valid = state->id.stringID;
            } else {
                assert(false);
            }
        } else {
            assert(false);
        }
    }
    return valid;
}


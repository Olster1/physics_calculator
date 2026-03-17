UndoRedoBlock *createUndoRedoSentinel() {
    UndoRedoBlock *result = pushStruct(&globalBoardValuesArena, UndoRedoBlock);
    result->isSentinel = true; 
    //NOTE: This isn't a real sentinel doubly link list, it's just a dummy block so we can undo without losing our list. 
    //So when we've unwinded the undo stack all the way, we are on this node
    return result;
}

BoardValue *getBoardValueBySingleIndex(BoardState *boardState, int index) {
    if(index >= 0 && index < boardState->boardSize*boardState->boardSize) {
        return &boardState->boardValues[index];
    } else {
        assert(false);
        return 0;
    }
}

BoardValue *getBoardValue(BoardState *boardState, int x, int y) {
    if(x >= 0 && x < boardState->boardSize && y >= 0 && y < boardState->boardSize) {
        return &boardState->boardValues[y*boardState->boardSize + x];
    } else {
        assert(false);
        return 0;
    }
}

void addUndoRedoBlock(BoardState *state, BoardValue *boardValue, int cellIndex, int value) {
    UndoRedoBlock *block = 0;

    //NOTE: Need to remove the undo blocks that aren't valid anymore because we're inserting a new one. Go to the beginning of the undo list and just move it across to the free list
    //      Becuse they're already pointing to each other, so only have to change the last one on the
    //      the list
    UndoRedoBlock *startBlock = state->undoRedoBlock;
    UndoRedoBlock *endBlock = state->undoRedoBlock;
    UndoRedoBlock *onePastStartBlock = startBlock->prev;
    int blockCount = 0;
    while(endBlock->prev) {
        endBlock = endBlock->prev;
        blockCount++;
    }
    
    if(endBlock != startBlock) {
        //NOTE: Now clear the the starting block
        startBlock->prev = 0;
        onePastStartBlock->next = state->undoRedoBlockFreeList;
        state->undoRedoBlockFreeList = endBlock;
    } else {
        assert(!startBlock->prev);
    }
    /////////////////////// END OF REMOVING UNUSED UNDOBLOCKS //////

    if(state->undoRedoBlockFreeList) {
        block = state->undoRedoBlockFreeList;
        state->undoRedoBlockFreeList = state->undoRedoBlockFreeList->next;
        easyPlatform_clearMemory(block, sizeof(UndoRedoBlock));
    } else {
        block = pushStruct(&globalBoardValuesArena, UndoRedoBlock);
    }

    block->cellIndex = cellIndex;
    block->value = value;
    block->prevValue = boardValue->value;

    if(state->undoRedoBlock) {
        state->undoRedoBlock->prev = block;
    }
    block->next = state->undoRedoBlock;
    state->undoRedoBlock = block;
}   


void setBoardValueCell(BoardState *boardState, int cellIndex, BoardValue *value, int cellValue, SoundAssets *soundAssets, SettingsToSave *settingsToSave) {
    addUndoRedoBlock(boardState, value, cellIndex, cellValue);
    value->value = cellValue;

    if(cellValue >= 1) {
        value->alertTimer = 0;
        playSoundForeground(&soundAssets->placeSound);
    } else {
        playSoundForeground(&soundAssets->eraseSound);
    }

    saveGame(boardState, settingsToSave);
}

void setMathGroupPartner(BoardState *state, int x, int y, int groupId) {
    if(x >= 0 && x < state->boardSize && y >= 0 && y < state->boardSize) {
        state->mathGroups.mathGroupIds[y*state->boardSize + x] = groupId;
    }
}

int getMathGroupPartner(BoardState *state, int x, int y) {
    if(x >= 0 && x < state->boardSize && y >= 0 && y < state->boardSize) {
        return state->mathGroups.mathGroupIds[y*state->boardSize + x];
    } else {
        return -1;
    }
}

struct MathGroupResult {
    int groupId;
    MathGroupPartner *group;
};

MathGroupResult addMathIdToGroup(BoardState *state, BoardMathType type) {
    MathGroupResult result = {};
    result.groupId = -1;
    

    assert(state->mathGroups.typeCount < MAX_MATH_GROUPS);
    if(state->mathGroups.typeCount < MAX_MATH_GROUPS) {
        result.groupId = ++state->mathGroups.typeCount; //NOTE: 0 is reserved for unassigned 
        MathGroupPartner partner = {};
        partner.type = type;
        state->mathGroups.mathTypes[result.groupId - 1] = partner;
        result.group = &state->mathGroups.mathTypes[result.groupId - 1];

        if(result.group) {
            result.group->farthestPosition.x = FLT_MAX;
            result.group->farthestPosition.y = -FLT_MAX;
        }
    }
    return result;
}

struct GroupCoord {
    int x;
    int y;
    bool valid;
};

bool tryComputeValue(BoardState *state, int boardIndex, int boardSize) {
    if(boardIndex >= (boardSize*boardSize)) {
        return true;
    }
    bool satisfyConstraint = false;

    int possibleValuesCount = 0;
    int *possibleValues = pushArray(&globalPerFrameArena, boardSize, int); //NOTE: This is the integer values that a cell can be. We remove numbers if it's already in a row or column yet

    for(int i = 0; i < boardSize; i++) {
        possibleValues[possibleValuesCount++] = i + 1;
    }

    int x = boardIndex % boardSize;
    int y = boardIndex / boardSize;


    const int dimensionCount = 2; //NOTE: we only have 2 dimensions to check: x & y
    for(int k = 0; k < dimensionCount; k++) {
        for(int i = 0; i < boardSize; i++) {
            BoardValue *value = 0;
            if(k == 0) {
                value = &state->boardValues[y*state->boardSize + i];
            } else {
                value = &state->boardValues[i*state->boardSize + x];
            }
            assert(value);
            
            if(value->targetValue > 0) {
                int integerValue = value->targetValue;

                //NOTE: Check if it's in the array, if it its - remove it.
                bool addedAlready = false;
                for(int j = 0; j < possibleValuesCount && !addedAlready; ++j) {
                    if(possibleValues[j] == integerValue) {
                        possibleValues[j] = possibleValues[--possibleValuesCount];
                        addedAlready = true;
                        break;
                    }
                }
            }
        }
    }

    if(possibleValuesCount <= 0) {
        //NOTE: Not a valid result, so need to backtrack the board
        state->boardValues[boardIndex].targetValue = -1;
        satisfyConstraint = false;
    } else {
        for(int i = 0; i < possibleValuesCount && !satisfyConstraint; ) {
            int possibleValueIndex = random_between_int(0, possibleValuesCount);
            assert(possibleValueIndex < possibleValuesCount);

            //NOTE: Set the target value now
            state->boardValues[boardIndex].targetValue = possibleValues[possibleValueIndex];

            //NOTE: Remove this value as being tried now
            possibleValues[possibleValueIndex] = possibleValues[--possibleValuesCount];

            if(tryComputeValue(state, boardIndex + 1, boardSize)) { 
                //NOTE: Satisfied successfully or end of board
                satisfyConstraint = true; 
            } else {
                state->boardValues[boardIndex].targetValue = -1;
            }
        }
    }
    return satisfyConstraint;
}

struct GroupSizeCount {
    int cellSizes[4];
};

struct FloodFillMathGroups {
    int x;
    int y;

    FloodFillMathGroups *nextValidGroup; //NOTE: This is becuase this node can be part of two lists: the sentinel list to check & the ones that are valid

    FloodFillMathGroups *next;
    FloodFillMathGroups *prev;
};

FloodFillMathGroups *allocateAndAddFloodFillMathGroup(BoardState *state, FloodFillMathGroups *sentinel, int x, int y, bool *visited) {
    FloodFillMathGroups *result = 0;
    int index = y*state->boardSize + x;
    if(!sentinel || (x >= 0 && x < state->boardSize && y >= 0 && y < state->boardSize && !visited[index])) { 
        result = pushStruct(&globalPerFrameArena, FloodFillMathGroups);

        result->x = x;
        result->y = y;
        

        if(sentinel) {
            visited[index] = true;
            
            FloodFillMathGroups *last = sentinel->prev;

            result->next = sentinel;
            result->prev = last;

            last->next = result;
            sentinel->prev = result;
        }
    }

    return result;
}

int getMathGroupTotal(int total, BoardMathType mathType, int value, int indexInParentArray) {
    if(mathType == BOARD_MATH_TYPE_ADD) {
        total += value;
    } else if(mathType == BOARD_MATH_TYPE_SUBTRACT) {
        if(indexInParentArray == 0) {
            total = value;
        } else {
                //NOTE: To avoid negative numbers we get the heightest number first
            if(total > value) {
                total -= value;
            } else {
                total = value - total;
            }
        }
    } else if(mathType == BOARD_MATH_TYPE_MULTIPLY) {
            if(indexInParentArray == 0) {
            total = value;
        } else {
            total *= value;
        }
    } else if(mathType == BOARD_MATH_TYPE_DIVIDE) {
        //TODO: This can still do fractions
        if(indexInParentArray == 0) {
            total = value;
        } else {
            //NOTE: To avoid fractions we get the heightest number first
            if(total > value) {
                total /= value;
            } else {
                total = value / total;
            }
        }
    } else if(mathType == BOARD_MATH_TYPE_NONE) {
        total = value;
    }
    return total;
}

BoardMathType getMathTypeFromAvoidMathTypes(bool *avoidMathTypes, bool canDoDivide, bool canDoSubtract) {
    int mathCount = 0;
    BoardMathType mathTypes[4];
    if(!avoidMathTypes[0]) {
        mathTypes[mathCount++] = BOARD_MATH_TYPE_ADD;
    }
    if(!avoidMathTypes[1]) {
        mathTypes[mathCount++] = BOARD_MATH_TYPE_MULTIPLY;
    }
    if(!avoidMathTypes[2] && canDoSubtract) {
        mathTypes[mathCount++] = BOARD_MATH_TYPE_SUBTRACT;
    }
    if(!avoidMathTypes[3] && canDoDivide) {
        mathTypes[mathCount++] = BOARD_MATH_TYPE_DIVIDE;
    }

    //NOTE: Just default to add if can't do any other ones
    if(mathCount <= 0) {
        mathTypes[mathCount++] = BOARD_MATH_TYPE_ADD;
    }
    BoardMathType result = mathTypes[random_between_int(0, mathCount)];

    return result;
}

void floodFillForPartners(BoardState *state, GroupSizeCount *groupSizes, int targetSize, bool *avoidMathSymbols = 0) {
    if(state->mathGroups.remainingCellCoordsCount <= 0) {
        return;
    }
    MemoryArenaMark mark = takeMemoryMark(&globalPerFrameArena);

    int groupTargetIndex = targetSize - 1;

    int index = random_between_int(0, state->mathGroups.remainingCellCoordsCount);
    int boardIndex = state->mathGroups.remainingCellCoords[index];
    int x = boardIndex % state->boardSize;
    int y = boardIndex / state->boardSize;

    int shapePosCount = 0;
    const int maxShapeSize = 4;
    float2 *shapePositions = pushArray(&globalPerFrameArena, maxShapeSize, float2);
    bool *visited = pushArray(&globalPerFrameArena, state->boardSize*state->boardSize, bool);

    int count = 0;
    for(int y = 0; y < state->boardSize; ++y) {
        for(int x = 0; x < state->boardSize; ++x) {
            if(getMathGroupPartner(state, x, y) > 0) { //NOTE: Already used by a different group, so set it as already visited
                visited[state->boardSize*y + x] = true;
                count++;
            }
        }
    }
    assert(((state->boardSize*state->boardSize) - count) == state->mathGroups.remainingCellCoordsCount);

    //NOTE: Set the sentinel
    FloodFillMathGroups *floodList = allocateAndAddFloodFillMathGroup(state, 0, 0, 0, visited); //NOTE: Sentinel
    floodList->prev = floodList->next = floodList;

    allocateAndAddFloodFillMathGroup(state, floodList, x, y, visited);

    while(floodList->next != floodList && shapePosCount < targetSize) {
        FloodFillMathGroups *f = floodList->next;

        f->next->prev = floodList;
        floodList->next = f->next;

        if(getMathGroupPartner(state, f->x, f->y) == 0) {
            assert(shapePosCount < maxShapeSize);
            shapePositions[shapePosCount++] = make_float2(f->x, f->y);
        }

        allocateAndAddFloodFillMathGroup(state, floodList, x + 1, y, visited);
        allocateAndAddFloodFillMathGroup(state, floodList, x - 1, y, visited);
        allocateAndAddFloodFillMathGroup(state, floodList, x, y + 1, visited);
        allocateAndAddFloodFillMathGroup(state, floodList, x, y - 1, visited);
    }

    if(shapePosCount == targetSize) {
       
    } else if(shapePosCount < targetSize) {
        if(shapePosCount == 3) {
            if(groupSizes->cellSizes[2] > 0) { //NOTE: Has size three cells still
                groupTargetIndex = 2;
            } else if(groupSizes->cellSizes[0] > 0) { //NOTE: has size 1 cells left
                groupTargetIndex = 0;
                shapePosCount -= 2;
            } else {
                //NOTE: Default to cell size 2
                groupTargetIndex = 1;
                shapePosCount -= 1;
            }
        }
         if(shapePosCount == 2) {
           if(groupSizes->cellSizes[0] > 0) { //NOTE: has size 1 cells left
                groupTargetIndex = 0;
                shapePosCount -= 1;
            } else {
                //NOTE: Default to cell size 2
                groupTargetIndex = 1;
                
            }
        }
        if(shapePosCount == 1) {
            //NOTE: Default to cell size 1
            groupTargetIndex = 0;
            
        }
    } else {
        assert(false);
    }

    //NOTE: Take away the group size we made
    groupSizes->cellSizes[groupTargetIndex] = groupSizes->cellSizes[groupTargetIndex] - 1;

    //NOTE: Get the math operation now based on the final size
    BoardMathType mathType = BOARD_MATH_TYPE_NONE;
    BoardMathType mathTypes[] = { BOARD_MATH_TYPE_ADD, BOARD_MATH_TYPE_MULTIPLY, BOARD_MATH_TYPE_SUBTRACT, BOARD_MATH_TYPE_DIVIDE };

    if(shapePosCount == 2) {
        int operationCount = 4;

        BoardValue *v1 = getBoardValue(state, shapePositions[0].x, shapePositions[0].y);
        BoardValue *v2 = getBoardValue(state, shapePositions[1].x, shapePositions[1].y);

        bool canDoDivide = true;

        assert(v1 && v2);
        if(v1 && v2) {
            int values[2] = { v1->targetValue, v2->targetValue };
            if(values[0] < values[1]) {
                int temp = values[0];
                values[0] = values[1];
                values[1] = temp;
            }
            if((values[0] % values[1]) > 0) {
                //NOTE: Would be a fraction so remove the divide operation
                operationCount = 3;
                canDoDivide = false;
            }
        }

        if(avoidMathSymbols) {
            mathType = getMathTypeFromAvoidMathTypes(avoidMathSymbols, canDoDivide, true);
        } else {
            mathType = mathTypes[random_between_int(0, operationCount)];
        }
    } else if(shapePosCount == 3 || shapePosCount == 4) {
        if(avoidMathSymbols) {
            mathType = getMathTypeFromAvoidMathTypes(avoidMathSymbols, false, false);
        } else {
            //NOTE: Only ADD & MULTIPLY
            mathType = mathTypes[random_between_int(0, 2)];
        }
    } 
    ////////////////////////////////////////////////

    MathGroupResult groupId = addMathIdToGroup(state, mathType);
    assert(groupId.groupId > 0);
    
    int total = 0;
    //NOTE: Remove the cells out of useable cells
    for(int j = 0; j < shapePosCount; ++j) {
        float2 pos = shapePositions[j];

        groupId.group->positions[groupId.group->size++] = pos;

        if(pos.x < groupId.group->farthestPosition.x) {
            groupId.group->farthestPosition = pos;
        }
        if(pos.y > groupId.group->farthestPosition.y) {
            groupId.group->farthestPosition = pos;
        }

        setMathGroupPartner(state, pos.x, pos.y, groupId.groupId);

        BoardValue *boardValue = getBoardValue(state, pos.x, pos.y);
        assert(boardValue);
        
        if(boardValue) {
            boardValue->mathGroupId = groupId.groupId;

            total = getMathGroupTotal(total, mathType, boardValue->targetValue, j);
        }

        int index = pos.y*state->boardSize + pos.x;

        int indexToRemove = -1;
        for(int i = 0; i < state->mathGroups.remainingCellCoordsCount && indexToRemove < 0; ++i) {
            if(state->mathGroups.remainingCellCoords[i] == index) {
                 indexToRemove = i;
                 break;
            }
        }
        if(indexToRemove >= 0) {
            state->mathGroups.remainingCellCoords[indexToRemove] = state->mathGroups.remainingCellCoords[--state->mathGroups.remainingCellCoordsCount];
        } else {
            assert(false);
        }
    }

    groupId.group->answer = total;

    releaseMemoryMark(&mark);

}

GroupSizeCount getGroupsSizesForBoardSize(int boardSize) {
    int boardArea = boardSize*boardSize;
    GroupSizeCount result = {};
    if(boardSize == 3) {
        result.cellSizes[0] = 1;
        //NOTE: Rest cell size 2
    } else if(boardSize == 4) {
        result.cellSizes[0] = 1;
        result.cellSizes[2] = 1;
        //NOTE: Rest cell size 2
    } else {
        result.cellSizes[3] = ceil(0.05*boardArea) / 4;
        result.cellSizes[2] = ceil(0.32*boardArea) / 3;
        result.cellSizes[0] = ceil(0.05*boardArea);
        //NOTE: Rest cell size 2
    }

    return result;
}

BoardState initBoardState(int boardSize, SoundAssets *soundAssets, SettingsToSave *settingsToSave,  bool *avoidMathSymbols, bool generateBoard) {
    refreshBoardValuesMemoryArena();
    BoardState state = {};

    state.id = makeEntityId();

    state.colorOffset = random_between_int(0, arrayCount(global_board_colors));

    state.numberChoiceMenuIdOpen = -1;
    state.numberChoiceAnimationTimerOpen = -1;

    state.zoomFactor = 1;

    state.undoRedoBlock = createUndoRedoSentinel();

    state.boardSize = boardSize;
    state.boardValues = pushArray(&globalBoardValuesArena, boardSize*boardSize, BoardValue);

    state.mathGroups.mathGroupIds = pushArray(&globalBoardValuesArena, boardSize*boardSize, int);
    state.mathGroups.remainingCellCoords = pushArray(&globalBoardValuesArena, boardSize*boardSize, int);
    
    for(int i = 0; i < boardSize*boardSize; ++i) {
        state.mathGroups.mathGroupIds[i] = 0; //NOTE: Zero for not allocated. -1 for invalid result from getMathGroupId
        state.mathGroups.remainingCellCoords[state.mathGroups.remainingCellCoordsCount++] = i;

        BoardValue *value = &state.boardValues[i];
        value->value = -1; //NOTE: -1 for not set, so don't render etc.
        value->alertTimer = -1;

        //NOTE: Ui state values
        value->animationFlyInTimer = 0; //NOTE: Start flying in
        // value->animationFlyInTimer = -1; //NOTE: Start flying in //DEBUG
        value->animationStartPosY = random_between_float(-40, -60);
        value->maxAnimationFlyInTimer = random_between_float(0.4f, 0.8f);
    }

    if(generateBoard) {

        ///////////// Generate the Board /////////////
        tryComputeValue(&state, 0, boardSize);

        //NOTE: Add the groups now
        // 1 cell → ceil(4%) → No operation
        // 2 cells → 50% → + - × ÷
        // 3 cells → 32% → + × 
        // 4 cells → 8% → + ×

        // 3 x 3 → just 1 x 1 cell and 2 cell
        // 4 x 4 → just 1 x 1 cell, 2 cell, 1 x 3 cell
        // 5 x 5 → just 1 cell, 2 cell, 1 x 3 cell

        GroupSizeCount groupSizes = getGroupsSizesForBoardSize(boardSize);

        while(state.mathGroups.remainingCellCoordsCount > 0 && groupSizes.cellSizes[3] > 0) {
            floodFillForPartners(&state, &groupSizes, 4, avoidMathSymbols);
        }
        while(state.mathGroups.remainingCellCoordsCount > 0 && groupSizes.cellSizes[2] > 0) {
            floodFillForPartners(&state, &groupSizes, 3, avoidMathSymbols);
        }
        while(state.mathGroups.remainingCellCoordsCount > 0 && groupSizes.cellSizes[0] > 0) {
            floodFillForPartners(&state, &groupSizes, 1, avoidMathSymbols);
        }
        while(state.mathGroups.remainingCellCoordsCount > 0) {
            floodFillForPartners(&state, &groupSizes, 2, avoidMathSymbols);
        }

        saveGame(&state, settingsToSave);
    }
    return state;
}

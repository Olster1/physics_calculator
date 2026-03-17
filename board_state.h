enum BoardMathType {
    BOARD_MATH_TYPE_NONE,
    BOARD_MATH_TYPE_ADD,
    BOARD_MATH_TYPE_SUBTRACT,
    BOARD_MATH_TYPE_MULTIPLY,
    BOARD_MATH_TYPE_DIVIDE,
    ////////
    BOARD_MATH_TYPE_COUNT,
};

struct MathGroupPartner {
    BoardMathType type;
    int answer;

    int size;
    float2 positions[4];
    float2 farthestPosition;
};

struct MathGroup {
    int *mathGroupIds; //NOTE: Allocated array of the board size with the math group ids corresponding to the math group types
    int *remainingCellCoords; //NOTE: Remaining cell that haven't been allocated
    int remainingCellCoordsCount; //NOTE: Running Size of the above array

    int typeCount;
    MathGroupPartner mathTypes[MAX_MATH_GROUPS];
};

struct BoardValue {
    int value; //NOTE: -1 not set
    float animationFlyInTimer;
    float maxAnimationFlyInTimer;
    float alertTimer; //NOTE: the timer that makes it look squishy when the user undos, redos, hints or sets a number
    float animationStartPosY;
    int targetValue; //NOTE: I create the board in reverse, so allocate the values then generate the sum rules. So this is a solution. But there could be more than one solution so have to check that aswell.
    int mathGroupId;
};

struct UndoRedoBlock {
    int cellIndex;
    int value;
    int prevValue;

    bool isSentinel;

    UndoRedoBlock *next;
    UndoRedoBlock *prev;
};

struct BoardState {
    EntityID id;
    int boardSize;
    int colorOffset;

    float zoomFactor; //NOTE: User can zoom in 
    float2 cameraPos;

    BoardValue *boardValues;
    UndoRedoBlock *undoRedoBlock;
    UndoRedoBlock *undoRedoBlockFreeList;

    int numberChoiceMenuIdOpen;
    float2 numberChoiceMenuPos;
    bool numberChoiceActivateThisFrame;
    float numberChoiceAnimationTimerOpen;

    float mathGroupFadeInTimer;

    MathGroup mathGroups;
};
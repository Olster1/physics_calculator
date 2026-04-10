
#define Kilobytes(size) (size*1024)
#define Megabytes(size) (Kilobytes(size)*1024)
#define Gigabytes(size) (Megabytes(size)*1024)

#define easyMemory_zeroStruct(memory, type) easyMemory_zeroSize(memory, sizeof(type))
#define easyMemory_zeroArray(array) easyMemory_zeroSize(array, sizeof(array))


typedef struct MemoryPiece MemoryPiece;
typedef struct MemoryPiece {
    void *memory;
    size_t totalSize; //size of just this memory block
    // size_t totalSizeOfArena; //size of total arena to roll back with
    size_t currentSize;

    MemoryPiece *next;

} MemoryPiece; //this is for the memory to remember

typedef struct {
    //NOTE: everything is in pieces now
    // void *memory;
    // unsigned int totalSize; //include all memory blocks
    // unsigned int totalCurrentSize;//total current size of all memory blocks
    int markCount;

    MemoryPiece *pieces; //actual details in the memory block
    MemoryPiece *piecesFreeList;

} Arena;

MemoryPiece *getCurrentMemoryPiece(Arena *arena) {
    MemoryPiece *piece = arena->pieces;
    assert(piece);
    return piece;

}

#define pushStruct(arena, type) (type *)pushSize(arena, sizeof(type))
#define pushStructAligned(arena, type, alignment) (type *)pushSize(arena, sizeof(type), alignment)

#define pushArray(arena, size, type) (type *)pushSize(arena, sizeof(type)*size)

void *pushSize(Arena *arena, size_t size, int alignment = 8) {
    uintptr_t safePadding = 0;
    if(alignment > 0 && arena->pieces) {
        MemoryPiece *piece = arena->pieces;
        u8 *ptr = (u8 *)piece->memory + piece->currentSize;
        uintptr_t misalignment = (uintptr_t)ptr % alignment;
        safePadding = (misalignment == 0) ? 0 : (alignment - misalignment);
    }

    size_t safeSize = size + safePadding;

    if(!arena->pieces || ((arena->pieces->currentSize + safeSize) > arena->pieces->totalSize)){ //doesn't fit in arena
        MemoryPiece *piece = arena->piecesFreeList; //get one of the free list

        size_t minSizeOfPiece = Kilobytes(1028);
        size_t extension = MathMax_sizet(minSizeOfPiece, size);
        if(piece)  {
            MemoryPiece **piecePtr = &arena->piecesFreeList;
            assert(piece->totalSize > 0);
            while(piece && piece->totalSize < extension) {//find the right size piece.
                piecePtr = &piece->next;
                piece = piece->next;
            }
            if(piece) {
                //take off list
                *piecePtr = piece->next;
                piece->currentSize = 0;
            }

        }

        if(!piece) {//need to allocate a new piece
            piece = (MemoryPiece *)calloc(sizeof(MemoryPiece), 1);
            piece->memory = calloc(extension, 1);
            piece->totalSize = extension;
            piece->currentSize = 0;
        }
        assert(piece);
        assert(piece->memory);
        assert(piece->totalSize > 0);
        assert(piece->currentSize == 0);

        //stick on list
        piece->next = arena->pieces;
        arena->pieces = piece;

        // piece->totalSizeOfArena = arena->totalSize;
        // assert((arena->currentSize_ + size) <= arena->totalSize);
    }

    MemoryPiece *piece = arena->pieces;

    uintptr_t padding = 0;
    if(alignment > 0) {
        u8 *ptr = (u8 *)piece->memory + piece->currentSize;
        uintptr_t misalignment = (uintptr_t)ptr % alignment;
        padding = (misalignment == 0) ? 0 : (alignment - misalignment);
    }

    assert(piece);
    assert((piece->currentSize + size) <= piece->totalSize);

    void *result = ((u8 *)piece->memory) + piece->currentSize + padding;
    piece->currentSize += (size + padding);

    easyMemory_zeroSize(result, (size + padding));
    return result;
}

Arena createArena(size_t size) {
    Arena result = {};
    pushSize(&result, size);
    assert(result.pieces);
    assert(result.pieces->memory);
    result.pieces->currentSize = 0;
    return result;
}

typedef struct {
    int id;
    Arena *arena;
    size_t memAt; //the actuall value we roll back, don't need to do anything else
    MemoryPiece *piece;
} MemoryArenaMark;

MemoryArenaMark takeMemoryMark(Arena *arena) {
    MemoryArenaMark result = {};
    result.arena = arena;
    result.memAt = arena->pieces->currentSize;
    result.id = arena->markCount++;
    result.piece = arena->pieces;
    return result;
}

void releaseMemoryMark(MemoryArenaMark *mark) {
    mark->arena->markCount--;
    Arena *arena = mark->arena;
    assert(mark->id == arena->markCount);
    assert(arena->markCount >= 0);
    assert(arena->pieces);
    //all ways the top piece is the current memory block for the arena.
    MemoryPiece *piece = arena->pieces;
    if(mark->piece != piece) {
        //not on the same memory block
        bool found = false;
        while(!found) {
            piece = arena->pieces;
            if(piece == mark->piece) {
                //found the right one
                found = true;
                break;
            } else {
                arena->pieces = piece->next;
                assert(arena->pieces);
                //put on free list
                piece->next = arena->piecesFreeList;
                arena->piecesFreeList = piece;
            }
        }
        assert(found);
    }
    assert(arena->pieces == mark->piece);
    //roll back size
    piece->currentSize = mark->memAt;
    assert(piece->currentSize <= piece->totalSize);
}

static Arena globalLongTermArena;
static Arena globalPerFrameArena;
static Arena globalBoardValuesArena;
static Arena globalPerVmRunLifetime;
static MemoryArenaMark boardValuesArenaMark;
static MemoryArenaMark perFrameArenaMark;
static MemoryArenaMark perVmArenaMark;

char *nullTerminateBuffer(char *result, char *string, int length) {
    for(int i = 0; i < length; ++i) {
        result[i]= string[i];
    }
    result[length] = '\0';
    return result;
}

#define nullTerminate(string, length) nullTerminateBuffer((char *)malloc(length + 1), string, length)
#define nullTerminateArena(string, length, arena) nullTerminateBuffer((char *)pushArray(arena, length + 1, char), string, length)

#define concat_withLength(a, aLength, b, bLength) concat_(a, aLength, b, bLength, 0)
#define concat(a, b) concat_(a, easyString_getSizeInBytes_utf8(a), b, easyString_getSizeInBytes_utf8(b), 0)
#define concatInArena(a, b, arena) concat_(a, easyString_getSizeInBytes_utf8(a), b, easyString_getSizeInBytes_utf8(b), arena)
char *concat_(char *a, s32 lengthA, char *b, s32 lengthB, Arena *arena) {
    int aLen = lengthA;
    int bLen = lengthB;

    int newStrLen = aLen + bLen + 1; // +1 for null terminator
    char *newString = 0;
    if(arena) {
        newString = (char *)pushArray(arena, newStrLen, char);
    } else {
        newString = (char *)easyPlatform_allocateMemory(newStrLen);
    }
    assert(newString);

    newString[newStrLen - 1] = '\0';

    char *at = newString;
    for (int i = 0; i < aLen; ++i)
    {
        *at++ = a[i];
    }

    for (int i = 0; i < bLen; ++i)
    {
        *at++ = b[i];
    }
    assert(at == &newString[newStrLen - 1]);
    assert(newString[newStrLen - 1 ] == '\0');

    return newString;
}

inline char *easy_createString_printf(Arena *arena, char *formatString, ...) {

    va_list args;
    va_start(args, formatString);

    char bogus[1];
    int stringLengthToAlloc = vsnprintf(bogus, 1, formatString, args) + 1; //for null terminator, just to be sure

    char *strArray = pushArray(arena, stringLengthToAlloc, char);

    vsnprintf(strArray, stringLengthToAlloc, formatString, args);

    va_end(args);

    return strArray;
}

void initMemoryArenas() {
    globalBoardValuesArena = createArena(Kilobytes(200));
    globalLongTermArena = createArena(Kilobytes(200));
    globalPerFrameArena = createArena(Kilobytes(100));
    globalPerVmRunLifetime = createArena(Megabytes(1));
    perFrameArenaMark = takeMemoryMark(&globalPerFrameArena);
    boardValuesArenaMark = takeMemoryMark(&globalBoardValuesArena);
    perVmArenaMark = takeMemoryMark(&globalPerVmRunLifetime);
}

void refreshBoardValuesMemoryArena() {
    releaseMemoryMark(&boardValuesArenaMark);
    boardValuesArenaMark = takeMemoryMark(&globalBoardValuesArena);
}

void refreshVmMemoryArena() {
    releaseMemoryMark(&perVmArenaMark);
    perVmArenaMark = takeMemoryMark(&globalPerVmRunLifetime);
}

void refreshPerFrameArena() {
    releaseMemoryMark(&perFrameArenaMark);
    perFrameArenaMark = takeMemoryMark(&globalPerFrameArena);
}


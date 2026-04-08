template <typename T>
struct List {
    T* data;
    int count;
    int capacity;
    Arena* arena;

    // Equivalent to initResizeArray / initResizeArrayArena
    static List<T> init(Arena* arena = nullptr, int initialCapacity = 1) {
        List<T> list = {};
        list.arena = arena;
        list.count = 0;
        list.capacity = initialCapacity;

        size_t totalSize = sizeof(T) * list.capacity;
        if (arena) {
            list.data = (T*)pushSize(arena, totalSize);
        } else {
            list.data = (T*)easyPlatform_allocateMemory(totalSize, EASY_PLATFORM_MEMORY_ZERO);
        }
        return list;
    }

    void release() {
        if (!arena && data) {
            easyPlatform_freeMemory(data);
        } else if (arena) {
            // As per your C code logic: we don't manually free arena memory
            assert(false && "Cannot manually free arena-allocated List");
        }
        data = nullptr;
        count = 0;
    }

    T* push(T item) {
        if (count == capacity) {
            int oldCapacity = capacity;
            capacity = round(capacity * 1.5f);
            if (capacity <= oldCapacity) capacity++;

            size_t oldSize = oldCapacity * sizeof(T);
            size_t newSize = capacity * sizeof(T);

            if (arena) {
                MemoryPiece* piece = getCurrentMemoryPiece(arena);
                u8* endOfArray = (u8*)data + oldSize;
                u8* endOfArena = (u8*)piece->memory + piece->currentSize;
                bool atEndOfArena = (endOfArray == endOfArena);

                size_t deltaBytes = newSize - oldSize;
                if (atEndOfArena && ((piece->currentSize + deltaBytes) <= piece->totalSize)) {
                    piece->currentSize += deltaBytes;
                } else {
                    T* newData = (T*)pushSize(arena, newSize);
                    easyPlatform_copyMemory(newData, data, oldSize);
                    data = newData;
                }
            } else {
                data = (T*)easyPlatform_reallocMemory(data, oldSize, newSize);
            }
        }

        T* target = &data[count];
        *target = item; // Typed assignment
        count++;
        return target;
    }

    void clear() {
        count = 0;
    }

    // Sugar: let's you use list[i] instead of pointer math
    T& operator[](int index) {
        assert(index >= 0 && index < count);
        return data[index];
    }
};
template <typename K, typename V>
struct Map {
    struct Node {
        K key;
        V value;
        Node* next;
    };

    Node** buckets;
    Node* freeList;
    int bucketCount;
    Arena* arena;

    static Map<K, V> init(Arena* arena = 0, int size = 4096) {
        Map<K, V> map = {};
        map.arena = arena;
        map.bucketCount = size;
        map.freeList = 0;

        size_t bucketsSize = sizeof(Node*) * map.bucketCount;
        if (arena) {
            map.buckets = (Node**)pushSize(arena, bucketsSize);
            memset(map.buckets, 0, bucketsSize);
        } else {
            map.buckets = (Node**)easyPlatform_allocateMemory(bucketsSize, EASY_PLATFORM_MEMORY_ZERO);
        }
        return map;
    }

    template <typename T>
    uint32_t hash_key(T key) {
        return get_crc32((char*)&key, sizeof(T));
    }

    template <>
    uint32_t hash_key<char*>(char* key) {
        uint32_t hash = 0;
        while (*key) {
            hash = (hash * 31) + *key++;
        }
        return hash;
    }

    template <typename M>
    bool keys_match(M a, M b) {
        return memcmp(&a, &b, sizeof(M)) == 0;
    }

    template <>
    inline bool keys_match<char*>(char* a, char* b) {
        return strcmp(a, b) == 0;
    }

    uint32_t getHash(K key) {
        // Now calls the correct version based on type K
        return hash_key(key) % bucketCount;
    }

    void insert(K key, V value) {
        uint32_t index = getHash(key);
        Node* head = buckets[index];

        // 1. Check if key already exists (Update)
        Node* curr = head;
        while (curr) {
            if (keys_match(curr->key, key)) {
                curr->value = value;
                return;
            }
            curr = curr->next;
        }

        // 2. Get a Node (Recycle or Allocate)
        Node* newNode = 0;
        if (freeList) {
            // Take the top node from the free list
            newNode = freeList;
            freeList = freeList->next;
            // Clear the memory just in case
            memset(newNode, 0, sizeof(Node));
        } else {
            // Nothing to recycle, ask the Arena/Malloc
            if (arena) {
                newNode = (Node*)pushSize(arena, sizeof(Node));
            } else {
                newNode = (Node*)easyPlatform_allocateMemory(sizeof(Node), EASY_PLATFORM_MEMORY_ZERO);
            }
        }

        // 3. Fill and Link
        newNode->key = key;
        newNode->value = value;
        newNode->next = head;
        buckets[index] = newNode;
    }

    bool remove(K key) {
        uint32_t index = getHash(key);
        Node* curr = buckets[index];
        Node* prev = 0;

        while (curr) {
            if (keys_match(curr->key, key)) {
                // Unplug from the bucket chain
                if (prev) {
                    prev->next = curr->next;
                } else {
                    buckets[index] = curr->next;
                }

                // Add to the Free List for future use
                curr->next = freeList;
                freeList = curr;

                return true;
            }
            prev = curr;
            curr = curr->next;
        }
        return false;
    }

    V* get(K key) {
        uint32_t index = getHash(key);
        Node* curr = buckets[index];
        while (curr) {
            if (keys_match(curr->key, key)) {
                return &curr->value;
            }
            curr = curr->next;
        }
        return 0;
    }
};



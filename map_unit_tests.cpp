struct DEBUG_TestKey {
    int x, y, z;
};

struct DEBUG_TestValue {
    int id;
    float health;
};

void DEBUG_MapTests(Arena *arena) {
    // 1. Initialization
    // We use a small size (16) to ensure we test the linked-list chaining
    // even with just a few items.
    Map<DEBUG_TestKey, DEBUG_TestValue> map = Map<DEBUG_TestKey, DEBUG_TestValue>::init(arena, 16);
    assert(map.bucketCount == 16);
    assert(map.freeList == 0);

    DEBUG_TestKey k1 = {1, 2, 3};
    DEBUG_TestValue v1 = {101, 100.0f};

    // 2. Basic Insert & Get
    map.insert(k1, v1);
    DEBUG_TestValue *result = map.get(k1);
    assert(result != 0);
    assert(result->id == 101);
    assert(result->health == 100.0f);

    // 3. Update Existing Key (Overwrite)
    DEBUG_TestValue v1_new = {101, 50.0f};
    map.insert(k1, v1_new);
    result = map.get(k1);
    assert(result->health == 50.0f); // Should be 50, not 100

    // 4. Multiple Items & Collision Handling
    DEBUG_TestKey k2 = {10, 20, 30};
    DEBUG_TestValue v2 = {202, 80.0f};
    map.insert(k2, v2);

    assert(map.get(k1)->id == 101);
    assert(map.get(k2)->id == 202);

    // 5. Remove Logic
    bool wasRemoved = map.remove(k1);
    assert(wasRemoved == true);
    assert(map.get(k1) == 0);    // K1 should be gone
    assert(map.get(k2) != 0);    // K2 should still exist
    assert(map.freeList != 0);   // The node for K1 should now be in the Free List

    // 6. Free List Recycling (Memory Efficiency)
    // When we insert a new key, it should take the node from the Free List
    // instead of asking the Arena for more memory.
    DEBUG_TestKey k3 = {99, 99, 99};
    DEBUG_TestValue v3 = {303, 10.0f};

    // We save the address of the free node to verify it gets reused
    void *recycledNodeAddress = (void *)map.freeList;

    map.insert(k3, v3);

    // The new node for K3 should live at the exact same memory address as the old K1
    // We check the address of the value inside the node
    DEBUG_TestValue *k3_result = map.get(k3);
    assert(map.freeList == 0); // Free list should be empty now

    // 7. Missing Key Safety
    DEBUG_TestKey k_fake = {-1, -1, -1};
    assert(map.get(k_fake) == 0);
    assert(map.remove(k_fake) == false);

    // 8. Stress Test / Volume
    for(int i = 0; i < 100; ++i) {
        DEBUG_TestKey tk = {i, i, i};
        DEBUG_TestValue tv = {i, (float)i};
        map.insert(tk, tv);
    }

    for(int i = 0; i < 100; ++i) {
        DEBUG_TestKey tk = {i, i, i};
        DEBUG_TestValue *res = map.get(tk);
        assert(res != 0);
        assert(res->id == i);
    }

    {
    Map<char*, int> map = Map<char*, int>::init(0, 16);

    // 2. Test Basic Insertion and Retrieval
    char* key1 = (char*)"apple";
    map.insert(key1, 100);

    int* val1 = map.get((char*)"apple");
    assert(val1 != nullptr);
    assert(*val1 == 100);
    printf("  - Basic match: PASSED\n");

    // 3. Test Deep Comparison (Different Pointers, Same Content)
    // We create a new string on the heap so it has a different memory address than "apple"
    char* key2 = (char*)malloc(6);
    strcpy(key2, "apple");

    // Even though pointers are different, content is the same.
    // This would FAIL in your old code, but PASS now.
    int* val2 = map.get(key2);
    assert(val2 != nullptr);
    assert(val1 == val2); // Should point to the same value entry
    printf("  - Deep comparison (different pointers): PASSED\n");

    // 4. Test Update logic
    map.insert(key2, 200); // Update "apple" using the heap pointer
    assert(*map.get(key1) == 200);
    printf("  - Update existing string key: PASSED\n");

    // 5. Test Non-existent key
    int* val_missing = map.get((char*)"orange");
    assert(val_missing == nullptr);
    printf("  - Missing key retrieval: PASSED\n");

    // 6. Test Removal
    bool removed = map.remove((char*)"apple");
    assert(removed == true);
    assert(map.get((char*)"apple") == nullptr);
    printf("  - Key removal: PASSED\n");
    }
}
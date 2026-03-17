enum EasyUi_Type {
    EASY_UI_WINDOW,
    EASY_UI_CHECKBOX,
    EASY_UI_TEXT,
};

struct EasyUi_Id {
    u32 crcHash;
    u32 hashIndex;
    char *id;
};

struct EasyUi_Element {
    EasyUi_Id id;
    EasyUi_Type type;

    float2 pos;
    float2 scale;
    float2 cursorAt;

    EasyUi_Element *next;
};

#define EASY_UI_ELEMENTS_HASH_SIZE 256

bool easyUi_idsMatch(EasyUi_Id a, EasyUi_Id b) {
    bool result = false;
    if(a.crcHash == b.crcHash) {
        if(easyString_stringsMatch_nullTerminated(a.id, b.id)) {
            result = true;
        }
    }
    return result;
}

EasyUi_Id easyUi_getId_(char *id) {
    EasyUi_Id result = {};
    result.id = id;
    result.crcHash = get_crc32(id, easyString_getSizeInBytes_utf8(id));
    result.hashIndex = result.crcHash % EASY_UI_ELEMENTS_HASH_SIZE;

    return result;
}

struct EasyUi_Renderer{
    EasyUi_Id id; //NOTE: Parent id that owns this 
    Renderer *renderer; //NOTE: Allocated per frame on per frame arena
};

struct EasyUi_HoverId{
    int layerIndex;
    int widgetIndex;
};

struct EasyUi_State {
    EasyUi_Element *currentWindow;
    EasyUi_Element *elements[EASY_UI_ELEMENTS_HASH_SIZE];

    EasyUi_Renderer renderers[256];
    int rendererCount;

    EasyUi_HoverId currentHoverId;
    EasyUi_HoverId hoverId;

    int layerIndex; //NOTE: Incremented per window
    int widgetIndex;//NOTE: Incremented per widget
};

/*NOTE: You call this before you see if a button is clicked but is in bounds. This says the button
        Is being hovered over.*/
bool easyUi_isHoverAllowed(EasyUi_State *state) {
    bool result = false;

    EasyUi_HoverId hoverId = {};
    hoverId.layerIndex = state->layerIndex;
    hoverId.widgetIndex = state->widgetIndex;
    
    if(state->hoverId.layerIndex <= hoverId.layerIndex && state->hoverId.widgetIndex <= hoverId.widgetIndex) {
        state->currentHoverId = hoverId;
        result = true;
    } 
    return result;
}

/*NOTE: Must be called at the end of the frame to transfer the hover state and clear the current hover state. 
This is important for hit testing to function correctly. 
*/
void easyUi_clearHoverId(EasyUi_State *state) {
    state->hoverId = state->currentHoverId;

    easyMemory_zeroStruct(&state->currentHoverId, EasyUi_HoverId);
    state->layerIndex = 0;
    state->widgetIndex = 0;
}

EasyUi_Element *easyUi_getElementById(EasyUi_State *state, EasyUi_Id id) {
    EasyUi_Element *elm = state->elements[id.hashIndex];
    bool found = false;

    while(elm && !found){
        if(easyUi_idsMatch(id, elm->id)) {
            found = true;
        } else {
            elm = elm->next;
        }
    } 

    state->widgetIndex++;

    return elm;
}

#define easyUi_getId() easyUi_getId_(##__FILE__##__LINE__##__FUNCTION__)

#define easyUi_window(state, name, pos) easyUi_window_(state, ##__FILE__##__LINE__##__FUNCTION__, name, pos)
void easyUi_window_(EasyUi_State *state, char *idString, char *name, float2 startPos) {
    EasyUi_Id id = easyUi_getId_(idString);

    EasyUi_Element *element = easyUi_getElementById(state, id);

    if(element) {
        element = pushStruct(&globalLongTermArena, EasyUi_Element);
    }
}

#define easyUi_text(text) easyUi_text_(##__FILE__##__LINE__##__FUNCTION__, text)
void easyUi_text_(char *id, char *text) {
    
}

#define easyUi_checkbox(value, label) easyUi_checkbox_(##__FILE__##__LINE__##__FUNCTION__, value, label)
bool easyUi_checkbox_(char *id, bool value, char *label) {
    return true;
}
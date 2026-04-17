struct AtlasAsset {
    char *name;
    float4 uv;
    float aspectRatio_h_over_w;

    AtlasAsset *next;
};

struct TextureAtlas {
    Texture *texture;

    //NOTE: Hash table of assets
    AtlasAsset *items[4096];
};


    AtlasAsset *textureAtlas_addItem(TextureAtlas *atlas, char *name, float4 uv) {
        uint32_t hash = get_crc32(name, easyString_getSizeInBytes_utf8(name));

        hash %= arrayCount(atlas->items);
        assert(hash < arrayCount(atlas->items));

        AtlasAsset **aPtr = &atlas->items[hash];

        while(*aPtr) {
            aPtr = &((*aPtr)->next);
        }

        assert((*aPtr) == 0);

        AtlasAsset *a = pushStruct(&globalLongTermArena, AtlasAsset);

        a->name = name;
        a->uv = uv;
        a->aspectRatio_h_over_w = safeDivide(uv.w - uv.y, uv.z - uv.x);

        a->next = 0;

        *aPtr = a;

        return a;
    }

    AtlasAsset *textureAtlas_getItem(TextureAtlas *atlas, char *name) {
        AtlasAsset *result = 0;

        uint32_t hash = get_crc32(name, easyString_getSizeInBytes_utf8(name));

        uint32_t hashIndex = hash % arrayCount(atlas->items);
        assert(hashIndex < arrayCount(atlas->items));

        AtlasAsset *a = atlas->items[hashIndex];

        while(a && !result) {
            uint32_t hashText = get_crc32(a->name, easyString_getSizeInBytes_utf8(a->name));
            if(hashText == hash && easyString_stringsMatch_nullTerminated(a->name, name)) {
                result = a;
            }

            a = a->next;
        }

        return result;
    }


    Texture *textureAtlas_getItemAsTexture(TextureAtlas *atlas, char *name, Arena *arena) {
        Texture *t = pushStruct(arena, Texture);
        AtlasAsset *i = textureAtlas_getItem(atlas, name);
        assert(i);
        if(i) {
            //NOTE: Fill out the texture details
            t->handle = atlas->texture->handle;
            float wPercent = (i->uv.z - i->uv.x);
            float hPercent = (i->uv.w - i->uv.y);
            t->width = atlas->texture->width*wPercent;
            t->height = atlas->texture->height*hPercent;
            // t.aspectRatio_h_over_w = i->aspectRatio_h_over_w;
            t->uv = i->uv;
        }
        return t;
    }

float textureAtlas_getNextFloat(EasyTokenizer *tokenizer) {
    EasyToken t = lexGetNextToken(tokenizer);
    float result = 0;
    if(t.type == TOKEN_FLOAT) {
        result = t.floatVal;
    } else if(t.type == TOKEN_MINUS) {
        t = lexSeeNextToken(tokenizer);
        if(t.type == TOKEN_FLOAT) {
            result = -1 * t.floatVal;
            //NOTE: Consume the token
            lexGetNextToken(tokenizer);
        }
    }
    return result;
}


    TextureAtlas readTextureAtlas(char *jsonFileName, char *textureFileName) {
        TextureAtlas result = {};

        FileContents contents  = platformReadEntireFile(&globalPerFrameArena, jsonFileName, true);
        void *memory = contents.memory;
        size_t data_size = contents.fileSize;

        assert(data_size > 0);
        assert(memory);

        EasyTokenizer tokenizer = lexBeginParsing(memory, EASY_LEX_OPTION_EAT_WHITE_SPACE);

        bool parsing = true;
        while(parsing) {
            EasyToken t = lexGetNextToken(&tokenizer);

            if(t.type == TOKEN_NULL_TERMINATOR) {
                parsing = false;
            } else if(t.type == TOKEN_OPEN_BRACKET) {
                //NOTE: Get the item out
                float4 uv = make_float4(0, 0, 0, 0);

                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_STRING);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_COLON);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_STRING);
                char *assetName = nullTerminateArena(t.at, t.size, &globalLongTermArena);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_COMMA);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_STRING);
                t = lexGetNextToken(&tokenizer);
                assert(t.type == TOKEN_COLON);

                uv.x = textureAtlas_getNextFloat(&tokenizer);
                uv.y = textureAtlas_getNextFloat(&tokenizer);
                uv.z = textureAtlas_getNextFloat(&tokenizer);
                uv.w = textureAtlas_getNextFloat(&tokenizer);

                textureAtlas_addItem(&result, assetName, uv);
            }
        }

        result.texture = platform_loadImage(textureFileName, &globalLongTermArena);

        return result;
    }

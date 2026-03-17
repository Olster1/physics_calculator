struct TextureHandle {
    void *handle;
};

struct Texture {
    int width;
    int height;

    float4 uv;

    TextureHandle handle;
};

float3 global_quadData[4] = {
    make_float3(-0.5f, -0.5f, 0), 
    make_float3(-0.5f, 0.5f, 0), 
    make_float3(0.5f, 0.5f, 0),
    make_float3(0.5f, -0.5f, 0),};

static Texture *global_white_image;

enum RenderItemType {
    RENDER_TEXTURE,
    RENDER_VIEW_MATRIX,
    RENDER_RECT,
    RENDER_GLYPH,
};

struct RenderItem {
    RenderItemType type;
    Texture *texture;
    float4 uvCoords;
    float16 T;
    float4 color;
};

struct Renderer {
    int commandsCount;
    RenderItem renderCommands[16384];
};
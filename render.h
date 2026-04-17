struct TextureHandle {
    void *handle;
};

struct Texture {
    int width;
    int height;

    float4 uv;

    TextureHandle handle;
};

struct Vertex {
    float3 pos;
    float2 texUV;
    float3 normal;
};

Vertex makeVertex(float3 pos, float2 texUV, float3 normal) {
    Vertex v = {};

    v.pos = pos;
    v.texUV = texUV;
    v.normal = normal;

    return v;
}

static Vertex global_quadData[] = {
    makeVertex(make_float3(0.5f, -0.5f, 0), make_float2(1, 1), make_float3(0, 0, 1)),
    makeVertex(make_float3(-0.5f, -0.5f, 0), make_float2(0, 1), make_float3(0, 0, 1)),
    makeVertex(make_float3(-0.5f,  0.5f, 0), make_float2(0, 0), make_float3(0, 0, 1)),
    makeVertex(make_float3(0.5f, 0.5f, 0), make_float2(1, 0), make_float3(0, 0, 1)),
};

static unsigned int global_quadIndices[] = {
    0, 1, 2, 0, 2, 3,
};

static unsigned int global_lineIndicies[] = {
    0, 1
};

static Vertex global_lineModelData[] = {
    makeVertex(make_float3(-0.5f, 0, 0), make_float2(0, 1), make_float3(0, 0, 1)),
    makeVertex(make_float3(0.5f, 0, 0), make_float2(1, 1), make_float3(0, 0, 1)),

};

struct Shader {
    bool valid;
    uint32_t handle;
};

struct ModelBuffer {
    uint32_t handle;
    uint32_t instanceBufferhandle;
    int indexCount;
};

static Texture *global_white_image;

enum RenderItemType {
    RENDER_TEXTURE,
    RENDER_VIEW_MATRIX,
    RENDER_RECT,
    RENDER_GLYPH,
    RENDER_SHADER,
};

struct InstanceDataWithRotation {
    float16 T;
    float4 color;
    float4 uv;
};

struct RenderItem {
    RenderItemType type;
    InstanceDataWithRotation instance;
    Texture *texture;
    Shader *shader;
};

struct RendererCommands {
    int commandsCount;
    RenderItem renderCommands[16384];
};

struct Shaders {
    Shader quadTextureShader;
    Shader fontShader;
    Shader pixelArtShader;
};

struct BackendRenderer {
    Shaders shaders;
    ModelBuffer quadModel;
    ModelBuffer lineModel;
};

struct Renderer {
    Shaders *shaders;
    RendererCommands renderCommands;
};
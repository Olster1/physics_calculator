#include "../libs/GLAD/include/glad/glad.h"
#include "../libs/GLAD/src/glad.c"
#ifndef __APPLE__
#include <GL/gl.h>
#endif

#define VERTEX_ATTRIB_LOCATION 0
#define NORMAL_ATTRIB_LOCATION 1
#define UV_ATTRIB_LOCATION 2
//----------- After this is per instance variables -----------//
#define UVATLAS_ATTRIB_LOCATION 3
#define COLOR_ATTRIB_LOCATION 4
#define MODEL_TRANSFORM_ATTRIB_LOCATION 5

#include "../shaders/shaders_opengl.cpp"


#define renderCheckError() renderCheckError_(__LINE__, (char *)__FILE__)
void renderCheckError_(int lineNumber, char *fileName) {
    #define RENDER_CHECK_ERRORS 1
    #if RENDER_CHECK_ERRORS
    GLenum err = glGetError();
    if(err) {
        printf((char *)"GL error check: %x at %d in %s\n", err, lineNumber, fileName);
        assert(!err);
    }
    #endif
}


Texture *platform_loadImage(char *fileName, Arena *arena) {
    int width, height, channels;
    unsigned char* data = stbi_load(fileName, &width, &height, &channels, 4);

    if (!data) return 0;

    unsigned int texture;
    glGenTextures(1, &texture);
    renderCheckError();
    glBindTexture(GL_TEXTURE_2D, texture);
    renderCheckError();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    renderCheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    renderCheckError();

    glBindTexture(GL_TEXTURE_2D, 0);
    renderCheckError();

    Texture *result = pushStruct(arena, Texture);

    result->width = width;
    result->height = height;
    result->uv = make_float4(0, 0, 1, 1);
    result->handle.handle = (void *)(uintptr_t)texture;

    return result;
}

Texture *platform_loadImageFromData(u8 *data, int width, int height, int bytesPerPixel, Arena *arena) {
    unsigned int texture;
    glGenTextures(1, &texture);
    renderCheckError();
    glBindTexture(GL_TEXTURE_2D, texture);
    renderCheckError();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    renderCheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    renderCheckError();

    glBindTexture(GL_TEXTURE_2D, 0);
    renderCheckError();

    Texture *result = pushStruct(arena, Texture);

    result->width = width;
    result->height = height;
    result->uv = make_float4(0, 0, 1, 1);
    result->handle.handle = (void *)(uintptr_t)texture;

    return result;
}


Shader loadShader(char *vertexShader, char *fragShader) {
    Shader result = {};

    result.valid = true;

    GLuint vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
    renderCheckError();
    GLuint fragShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
    renderCheckError();

    glShaderSource(vertexShaderHandle, 1, (const GLchar **)(&vertexShader), 0);
    renderCheckError();
    glShaderSource(fragShaderHandle, 1, (const GLchar **)(&fragShader), 0);
    renderCheckError();

    glCompileShader(vertexShaderHandle);
    renderCheckError();
    glCompileShader(fragShaderHandle);
    renderCheckError();
    result.handle = glCreateProgram();
    renderCheckError();
    glAttachShader(result.handle, vertexShaderHandle);
    renderCheckError();
    glAttachShader(result.handle, fragShaderHandle);
    renderCheckError();

    int max_attribs;
    glGetIntegerv (GL_MAX_VERTEX_ATTRIBS, &max_attribs);
    renderCheckError();

    glBindAttribLocation(result.handle, VERTEX_ATTRIB_LOCATION, "vertex");
    renderCheckError();
    glBindAttribLocation(result.handle, NORMAL_ATTRIB_LOCATION, "normal");
    renderCheckError();
    glBindAttribLocation(result.handle, UV_ATTRIB_LOCATION, "texUV");
    renderCheckError();
    glBindAttribLocation(result.handle, UVATLAS_ATTRIB_LOCATION, "uvAtlas");
    renderCheckError();
    glBindAttribLocation(result.handle, COLOR_ATTRIB_LOCATION, "color");
    renderCheckError();
    glBindAttribLocation(result.handle, MODEL_TRANSFORM_ATTRIB_LOCATION, "M");
    renderCheckError();

    glLinkProgram(result.handle);
    renderCheckError();
    glUseProgram(result.handle);
    renderCheckError();

    GLint success = 0;
    glGetShaderiv(vertexShaderHandle, GL_COMPILE_STATUS, &success);
    renderCheckError();

    GLint success1 = 0;
    glGetShaderiv(fragShaderHandle, GL_COMPILE_STATUS, &success1);
    renderCheckError();

    if(success == GL_FALSE || success1 == GL_FALSE) {
        result.valid = false;
        int  vlength,    flength,    plength;
        char vlog[2048];
        char flog[2048];
        char plog[2048];
        glGetShaderInfoLog(vertexShaderHandle, 2048, &vlength, vlog);
        renderCheckError();
        glGetShaderInfoLog(fragShaderHandle, 2048, &flength, flog);
        renderCheckError();
        glGetProgramInfoLog(result.handle, 2048, &plength, plog);
        renderCheckError();


        if(vlength || flength || plength) {
            printf("%s\n", vertexShader);
            printf("%s\n", fragShader);
            printf("%s\n", vlog);
            printf("%s\n", flog);
            printf("%s\n", plog);

        }
    }

    assert(result.valid);

    return result;
}

static inline void addInstanceAttribForMatrix(int index, GLuint attribLoc, int numOfFloats, size_t offsetForStruct, size_t offsetInStruct) {
    glEnableVertexAttribArray(attribLoc + index);
    renderCheckError();

    glVertexAttribPointer(attribLoc + index, numOfFloats, GL_FLOAT, GL_FALSE, offsetForStruct, (void*)(uintptr_t)(offsetInStruct + (4 * sizeof(float) * index)));
    renderCheckError();
    glVertexAttribDivisor(attribLoc + index, 1);
    renderCheckError();
}

static inline void addInstancingAttrib (GLuint attribLoc, int numOfFloats, size_t offsetForStruct, size_t offsetInStruct) {
    assert(offsetForStruct > 0);
    if(numOfFloats == 16) {
        addInstanceAttribForMatrix(0, attribLoc, 4, offsetForStruct, offsetInStruct);
        renderCheckError();
        addInstanceAttribForMatrix(1, attribLoc, 4, offsetForStruct, offsetInStruct);
        renderCheckError();
        addInstanceAttribForMatrix(2, attribLoc, 4, offsetForStruct, offsetInStruct);
        renderCheckError();
        addInstanceAttribForMatrix(3, attribLoc, 4, offsetForStruct, offsetInStruct);
        renderCheckError();
    } else {
        glEnableVertexAttribArray(attribLoc);
        renderCheckError();

        assert(numOfFloats <= 4);
        glVertexAttribPointer(attribLoc, numOfFloats, GL_FLOAT, GL_FALSE, offsetForStruct, ((char *)0) + offsetInStruct);
        renderCheckError();

        glVertexAttribDivisor(attribLoc, 1);
        renderCheckError();
    }
}

void addInstancingAttribsForShader() {
    size_t offsetForStruct = sizeof(InstanceDataWithRotation);

    unsigned int uvOffset = (intptr_t)(&(((InstanceDataWithRotation *)0)->uv));
    addInstancingAttrib (UVATLAS_ATTRIB_LOCATION, 4, offsetForStruct, uvOffset);
    renderCheckError();
    unsigned int colorOffset = (intptr_t)(&(((InstanceDataWithRotation *)0)->color));
    addInstancingAttrib (COLOR_ATTRIB_LOCATION, 4, offsetForStruct, colorOffset);
    renderCheckError();
    unsigned int modelOffset = (intptr_t)(&(((InstanceDataWithRotation *)0)->T));
    addInstancingAttrib (MODEL_TRANSFORM_ATTRIB_LOCATION, 16, offsetForStruct, modelOffset);
    renderCheckError();
}

ModelBuffer generateVertexBuffer(void *triangleData, int vertexCount, unsigned int *indicesData, int indexCount) {
    ModelBuffer result = {};
    glGenVertexArrays(1, &result.handle);
    renderCheckError();
    glBindVertexArray(result.handle);
    renderCheckError();

    GLuint vertices;
    GLuint indices;

    glGenBuffers(1, &vertices);
    renderCheckError();

    glBindBuffer(GL_ARRAY_BUFFER, vertices);
    renderCheckError();

    size_t sizeOfVertex = sizeof(Vertex);

    glBufferData(GL_ARRAY_BUFFER, vertexCount*sizeOfVertex, triangleData, GL_STATIC_DRAW);
    renderCheckError();

    glGenBuffers(1, &indices);
    renderCheckError();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
    renderCheckError();

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount*sizeof(unsigned int), indicesData, GL_STATIC_DRAW);
    renderCheckError();

    result.indexCount = indexCount;

    //NOTE: Assign the attribute locations with the data offsets & types
    GLint vertexAttrib = VERTEX_ATTRIB_LOCATION;
    glEnableVertexAttribArray(vertexAttrib);
    renderCheckError();
    glVertexAttribPointer(vertexAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    renderCheckError();

    GLint texUVAttrib = UV_ATTRIB_LOCATION;
    glEnableVertexAttribArray(texUVAttrib);
    renderCheckError();
    unsigned int uvByteOffset = (intptr_t)(&(((Vertex *)0)->texUV));
    glVertexAttribPointer(texUVAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char *)0) + uvByteOffset);
    renderCheckError();

    GLint normalsAttrib = NORMAL_ATTRIB_LOCATION;
    glEnableVertexAttribArray(normalsAttrib);
    renderCheckError();
    unsigned int normalOffset = (intptr_t)(&(((Vertex *)0)->normal));
    glVertexAttribPointer(normalsAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char *)0) + normalOffset);
    renderCheckError();

    // vbo instance buffer
    {
        glGenBuffers(1, &result.instanceBufferhandle);
        renderCheckError();

        glBindBuffer(GL_ARRAY_BUFFER, result.instanceBufferhandle);
        renderCheckError();

        glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_DYNAMIC_DRAW);
        renderCheckError();

        addInstancingAttribsForShader();
        renderCheckError();
    }

    glBindVertexArray(0);
    renderCheckError();

    //we can delete these buffers since they are still referenced by the VAO
    glDeleteBuffers(1, &vertices);
    renderCheckError();
    glDeleteBuffers(1, &indices);
    renderCheckError();

    return result;
}

void backendRenderer_bindTexture2D(uint32_t handle) {
    glBindTexture(GL_TEXTURE_2D, handle);
    renderCheckError();
}

void backend_render_swapFrame(SDL_Window *hwnd) {
    SDL_GL_SwapWindow(hwnd);
    renderCheckError();
}

void backend_render_clearFrame(float4 color) {
    glClearColor(color.x, color.y, color.z, color.w);
    renderCheckError();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderCheckError();
}

void backendRenderer_setViewport(float x0, float y0, float x1, float y1) {
    glViewport(x0, y0, x1, y1);
}

void backend_render_getOutputSize(int *w, int *h) {
}

void backend_render_init(SDL_Window *hwnd, BackendRenderer *r) {
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    SDL_GLContext context = SDL_GL_CreateContext(hwnd);

    if(context) {
        if(SDL_GL_MakeCurrent(hwnd, context)) {

            if(SDL_GL_SetSwapInterval(1)) {
                // Success
            } else {
                printf("Couldn't set swap interval\n");
            }
        } else {
            printf("Couldn't make context current\n");
        }
    }

    if (!gladLoadGL()) {
      assert(false);
    }

    glEnable(GL_DEPTH_TEST);
    renderCheckError();
    glDepthFunc(GL_LEQUAL);
    renderCheckError();

    glEnable(GL_BLEND);
    renderCheckError();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    renderCheckError();

    r->shaders.quadTextureShader = loadShader(quadVertexShader, quadTextureFragShader);
    r->shaders.fontShader = loadShader(quadVertexShader, sdfFragShader);
    r->shaders.pixelArtShader = loadShader(quadVertexShader, pixelArtFragShader);

    r->quadModel = generateVertexBuffer(global_quadData, 4, global_quadIndices, 6);
    // r->lineModel = generateVertexBuffer(global_lineData, 2, global_lineIndices, 2);
}

void updateInstanceData(uint32_t bufferHandle, void *data, size_t sizeInBytes) {
    glBindBuffer(GL_ARRAY_BUFFER, bufferHandle);
    renderCheckError();
    glBufferData(GL_ARRAY_BUFFER, sizeInBytes, data, GL_STREAM_DRAW);
    renderCheckError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    renderCheckError();
}

void bindTextureForShader(char *uniformName, int slotId, GLint textureId, Shader *shader) {
    GLint texUniform = glGetUniformLocation(shader->handle, uniformName);
    renderCheckError();

    glUniform1i(texUniform, slotId);
    renderCheckError();
    glActiveTexture(GL_TEXTURE0 + slotId);
    renderCheckError();
    glBindTexture(GL_TEXTURE_2D, textureId);
    renderCheckError();
}

void drawModels(ModelBuffer *model, Shader *shader, uint32_t textureId, int instanceCount, float16 projectionTransform, float16 modelViewTransform, GLenum primitive = GL_TRIANGLES) {
    glUseProgram(shader->handle);
    renderCheckError();

    glBindVertexArray(model->handle);
    renderCheckError();

    glUniformMatrix4fv(glGetUniformLocation(shader->handle, "V"), 1, GL_FALSE, modelViewTransform.E);
    renderCheckError();
    glUniformMatrix4fv(glGetUniformLocation(shader->handle, "projection"), 1, GL_FALSE, projectionTransform.E);
    renderCheckError();

    bindTextureForShader("diffuse", 1, textureId, shader);
    renderCheckError();

    glDrawElementsInstanced(primitive, model->indexCount, GL_UNSIGNED_INT, 0, instanceCount);
    renderCheckError();

    glBindVertexArray(0);
    renderCheckError();
    glUseProgram(0);
    renderCheckError();

}

void processRenderGroup(Renderer *renderer, float2 viewPortSize, BackendRenderer *backendRenderer) {
    float16 currentViewMatrix = float16_identity();
    Shader *currentShader = &backendRenderer->shaders.quadTextureShader;
    for(int i = 0; i < renderer->renderCommands.commandsCount; ++i) {
        RenderItem *item = renderer->renderCommands.renderCommands + i;

        if(item->type == RENDER_TEXTURE) {
            updateInstanceData(backendRenderer->quadModel.instanceBufferhandle, &item->instance, 1*sizeof(InstanceDataWithRotation));
            drawModels(&backendRenderer->quadModel, currentShader, (u32)(uintptr_t)item->texture->handle.handle, 1, currentViewMatrix, float16_identity());
        } else if(item->type == RENDER_VIEW_MATRIX) {
            currentViewMatrix = item->instance.T;
        } else if(item->type == RENDER_SHADER) {
            currentShader = item->shader;
        }

    }
    renderer->renderCommands.commandsCount = 0;
}
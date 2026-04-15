static char *lineVertexShader =
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec3 normal;"
"in vec2 texUV;	"

//per instanced variables
"in mat4 M;"
"in vec4 uvAtlas;"
"in vec4 color;"

//uniform variables
"uniform mat4 V;"
"uniform mat4 projection;"
"uniform vec2 canvasDimInWorldUnits;"

//outgoing variables
"out vec4 color_frag;"
"out vec2 uv_frag;"

"void main() {"
    "mat4 MV = V * M;"
    "gl_Position = projection * MV * vec4((vertex), 1);"
    "color_frag = color;"
    "uv_frag =  ((M * vec4((vertex), 1)).xy + 0.5*canvasDimInWorldUnits) / canvasDimInWorldUnits;"
"}";

static char *lineVertexScreenSpaceShader =
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec3 normal;"
"in vec2 texUV;	"

//per instanced variables
"in mat4 M;"
"in vec4 uvAtlas;"
"in vec4 color;"

//uniform variables
"uniform mat4 V;"
"uniform mat4 projection;"
"uniform vec2 canvasDimInWorldUnits;"

//outgoing variables
"out vec4 color_frag;"
"out vec2 uv_frag;"

"void main() {"
    "mat4 MV = V * M;"
    "gl_Position = projection * MV * vec4((vertex), 1);"
    "color_frag = color;"
    "uv_frag =  ((M * vec4((vertex), 1)).xy + 0.5*canvasDimInWorldUnits) / canvasDimInWorldUnits;"
"}";


static char *lineFragShader =
"#version 330\n"
"in vec4 color_frag;"
"in vec2 uv_frag;"
"out vec4 color;"
"uniform sampler2D diffuse;" //NOTE: To get the inverse color
"void main() {"
    "vec4 colorSample = texture(diffuse, uv_frag);"
    "colorSample = vec4(1.0) - colorSample;"
    "colorSample.w = 1;"

    "if(color_frag.w == 0) {"
        "colorSample = color_frag;"
    "}"


    "color = colorSample;"
"}";


static char *quadVertexShader =
"#version 330\n"
//per vertex variables
"in vec3 vertex;"
"in vec3 normal;"
"in vec2 texUV;	"

//per instanced variables
"in mat4 M;"
"in vec4 uvAtlas;"
"in vec4 color;"

//uniform variables
"uniform mat4 V;"
"uniform mat4 projection;"

//outgoing variables
"out vec4 color_frag;"
"out vec2 uv_frag;"

"void main() {"
    "mat4 MV = V * M;"
    "gl_Position = projection * MV * vec4((vertex), 1);"
    "color_frag = color;"

   "uv_frag = vec2(mix(uvAtlas.x, uvAtlas.z, texUV.x), mix(uvAtlas.y, uvAtlas.w, texUV.y));"
"}";

static char *quadTextureFragShader =
"#version 330\n"
"in vec4 color_frag;"
"in vec2 uv_frag; "
"uniform sampler2D diffuse;"
"out vec4 color;"
"void main() {"
    "vec4 diffSample = texture(diffuse, uv_frag);"
    "if(diffSample.w == 0) {"
        "discard;"
    "}"
    "color = diffSample*color_frag;"
"}";

static char *fontTextureFragShader =
"#version 330\n"
"in vec4 color_frag;"
"in vec2 uv_frag; "
"uniform sampler2D diffuse;"
"out vec4 color;"
"void main() {"
    "vec4 diffSample = texture(diffuse, uv_frag);"
    "color = vec4(diffSample.r*color_frag);"
"}";

static char *quadFragShader =
"#version 330\n"
"in vec4 color_frag;"
"in vec2 uv_frag; "
"out vec4 color;"
"void main() {"
    "color = color_frag;"
"}";

static char *checkQuadFragShader =
"#version 330\n"
"in vec4 color_frag;"
"in vec2 uv_frag; "
"uniform sampler2D diffuse;"
"out vec4 color;"

"void main()"
"{"
    // Get the pixel size of the target texture
    "ivec2 size = textureSize(diffuse, 0);"

    // Compute checker width in pixels (same as w / 16)
    "int checkerWidth = size.x / 16;"

    // Compute checker indices
    "int x = int(float(size.x)*uv_frag.x);"
    "int y = int(float(size.y)*uv_frag.y);"

    "vec3 shadeColor = vec3(0.8);"
    "if((((y / checkerWidth) % 2) == 0 && ((x / checkerWidth) % 2) == 1) || (((y/ checkerWidth) % 2) == 1 && ((x/ checkerWidth) % 2) == 0)) {"
        "shadeColor = vec3(0.6);"
    "}"

    "color = vec4(shadeColor, 1.0);"
"}";
#define STB_TRUETYPE_IMPLEMENTATION
#include "./libs/stb_truetype.h"

struct Font {
    Texture *texture; //NOTE: Handle to the font atlas texture on the GPU  
    stbtt_bakedchar glyphData[255]; //NOTE: Meta data for each glyph like width and height and the uv coords in the texture atlas. In this case it's limited to be 255 characters per font. 
    float fontHeight;
    int startOffset;
    float2 fontAtlasDim;
};

Font initFontAtlas(unsigned char *ttfBuffer) {
    Font result = {};

    int tempBitmapWidth = 512;
    int tempBitmapHeight = 512;

    result.fontAtlasDim.x = tempBitmapWidth;
    result.fontAtlasDim.y = tempBitmapHeight;

    //NOTE: Allocate the CPU side image to render the glyphs to. We will then upload this to the GPU to use by our game renderer.
    unsigned char *tempBitmap = pushArray(&globalPerFrameArena, tempBitmapWidth*tempBitmapHeight, unsigned char);
    assert(tempBitmap);

    result.fontHeight = 64.0;
    result.startOffset = 32;
    //NOTE:  32, 96 values denote the main ASCI alphabet - starting at [SPACE] and going to the end of the asci table. The space is important because it tells us the width of a space.
    int r = stbtt_BakeFontBitmap(ttfBuffer, 0, result.fontHeight, tempBitmap, tempBitmapWidth, tempBitmapHeight, 32, 96, result.glyphData);

    if(r == 0) {
        printf("ERROR: Couldn't render font atlas\n");
        assert(false);
    } 

    u32 *tempBitmap4Bytes = pushArray(&globalPerFrameArena, tempBitmapWidth*tempBitmapHeight, u32);

    u32 white = 0x00FFFFFF;
    for(int y = 0; y < tempBitmapHeight; ++y) {
        for(int x = 0; x < tempBitmapWidth; ++x) {
            unsigned char alpha = tempBitmap[y*tempBitmapWidth + x];
            tempBitmap4Bytes[y*tempBitmapWidth + x] = (u32)(((u32)alpha) << 24) | white;
        }
    }

    result.texture = platform_loadImageFromData((u8 *)tempBitmap4Bytes, tempBitmapWidth, tempBitmapHeight, 4, &globalLongTermArena);

   return result;

}

//NOTE: Start is the baseline and starting horizontal point
Rect2f renderText(Renderer *renderer, Font *font, char *nullTerminatedString, float2 start, float scale, float4 color = make_float4(1, 1, 1, 1), bool render = true, int cursorIndex = -1, Rect2f *cursorSize = 0) {
    float x = 0;
    float y = 0;

    Rect2f bounds = make_rect2f_inverse_infinity();
    int charIndex = 0;

    while (*nullTerminatedString) {
        if(*nullTerminatedString != '\n') {
            if (*nullTerminatedString >= 32 && *nullTerminatedString < 126) {

                stbtt_aligned_quad q  = {};

                float beginY = y;
                float lastX = x;
                float lastY = y;

                int index = *nullTerminatedString - font->startOffset;
                stbtt_GetBakedQuad(font->glyphData, font->fontAtlasDim.x, font->fontAtlasDim.y, index, &x, &y, &q, 1);

                //NOTE: Because our renderer is center point based be get the middle of the glyph
                float width = scale*(q.x1 - q.x0);
                float height = scale*(q.y1 - q.y0);

                float x1 = scale*q.x0 + 0.5f*width;
                float y1 = scale*q.y0 + 0.5f*height;
                y1 = -y1; //NOTE: Flip the cooridnates because stb-font gives y axis positive down but our renderer is opposite

                x1 += start.x;
                y1 += start.y;
                
                float4 uvCoords = make_float4(q.s0, q.t0, q.s1, q.t1);

                float3 glyphScale = make_float3(width, height, 1);

                if(*nullTerminatedString == ' ') {
                    //NOTE: Get the width from the advance
                    glyphScale.x = scale*(x - lastX);
                    glyphScale.y = scale*(y - lastY);

                }
                Rect2f b = make_rect2f_center_dim(make_float2(x1, y1), glyphScale.xy);
                
                if(cursorSize && charIndex == (cursorIndex - 1)) {
                    *cursorSize = b;
                }
                
                bounds = rect2f_union(bounds, b);

                if(render) {
                    pushRenderGlyph(renderer, make_float3(x1, y1, 1), glyphScale, uvCoords, color, font->texture);
                }
            }
        } else {
            //NOTE: Move down a line
            x = 0;
            y -= font->fontHeight;
        }
        charIndex++;
        nullTerminatedString++;
    }

    return bounds;
}

Rect2f renderText_centered(Renderer *renderer, Font *font, char *nullTerminatedString, float2 start, float scale, float4 color = make_float4(1, 1, 1, 1), bool render = true) {
    Rect2f bounds = renderText(renderer, font, nullTerminatedString, start, scale, color, false);
    float2 rectScale = get_scale_rect2f(bounds);
    float2 offsetStart = start;

    // pushRenderTexture(renderer, make_transformX(make_float3(offsetStart.x, offsetStart.y, 1), make_float3(rectScale.x, rectScale.y, 1), make_float4(0, 0, 0, 1)), global_white_image, make_float4(1, 0, 0, 1));
    offsetStart.x -= 0.5f*rectScale.x;
    offsetStart.y -= 0.5f*rectScale.y;
    return renderText(renderer, font, nullTerminatedString, offsetStart, scale, color, true);
}

float2 renderText_getDim(Renderer *renderer, Font *font, char *nullTerminatedString, float scale) {
    Rect2f bounds = renderText(renderer, font, nullTerminatedString, make_float2(0, 0), scale, make_float4(0, 0, 0, 0), false);
    float2 rectScale = get_scale_rect2f(bounds);
    return rectScale;
}
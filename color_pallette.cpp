struct Editor_Color_Palette
{
	float4 background;
	float4 backgroundVariation;
	float4 standard;
	float4 variable;
	float4 bracket;
	float4 function;
	float4 keyword;
	float4 comment;
	float4 preprocessor;
	float4 string;
};

static inline float4 color_rgb255_to_01(float r, float g, float b) {
    float4 result = {};

    result.x = r / 255.0f; //red
    result.y = g / 255.0f;
    result.z = b / 255.0f;
    result.w = 1;
    return result;
}

//alpha is at 24 place
static inline float4 color_hexARGBTo01(unsigned int color) {
    float4 result = {};

    result.x = (float)((color >> 16) & 0xFF) / 255.0f; //red
    result.y = (float)((color >> 8) & 0xFF) / 255.0f;
    result.z = (float)((color >> 0) & 0xFF) / 255.0f;
    result.w = (float)((color >> 24) & 0xFF) / 255.0f;
    return result;
}

struct Color_Palettes {
	Editor_Color_Palette pallettes[6];
};

static Color_Palettes init_color_palettes() {

	Color_Palettes result = {};

	result.pallettes[0].background = color_hexARGBTo01(0xFF161616);
	result.pallettes[0].backgroundVariation = color_hexARGBTo01(0xFF3c3d3d);
	result.pallettes[0].standard =  color_hexARGBTo01(0xFFA08563);
	result.pallettes[0].variable = color_hexARGBTo01(0xFF6B8E23);
	result.pallettes[0].bracket = color_hexARGBTo01(0xFFDAB98F);
	result.pallettes[0].function = color_hexARGBTo01(0xFF008563);
	result.pallettes[0].keyword = color_hexARGBTo01(0xFFCD950C);
	result.pallettes[0].comment = color_hexARGBTo01(0xFF7D7D7D);
	result.pallettes[0].preprocessor = color_hexARGBTo01(0xFFDAB98F);
	result.pallettes[0].string = color_hexARGBTo01(0xFFDAB98F);

	result.pallettes[1].background = color_rgb255_to_01(6, 38, 38);
	result.pallettes[1].backgroundVariation = color_rgb255_to_01(6, 55, 55);
	result.pallettes[1].standard =  color_rgb255_to_01(194, 205, 187);
	result.pallettes[1].variable = color_rgb255_to_01(139, 194, 186);
	result.pallettes[1].bracket = color_rgb255_to_01(194, 205, 187);
	result.pallettes[1].function = color_rgb255_to_01(194, 205, 187);
	result.pallettes[1].keyword = color_rgb255_to_01(176, 200, 202);
	result.pallettes[1].comment = color_rgb255_to_01(92, 175, 87);
	result.pallettes[1].preprocessor = color_rgb255_to_01(118, 170, 138);
	result.pallettes[1].string = color_rgb255_to_01(118, 170, 138);

	// --- Mignight ---
	result.pallettes[2].background          = color_hexARGBTo01(0xFF0B0E14);
    result.pallettes[2].backgroundVariation = color_hexARGBTo01(0xFF1A1C23);
    result.pallettes[2].standard            = color_hexARGBTo01(0xFFABB2BF);
    result.pallettes[2].variable            = color_hexARGBTo01(0xFF00E5FF);
    result.pallettes[2].bracket             = color_hexARGBTo01(0xFFF170FF);
    result.pallettes[2].function            = color_hexARGBTo01(0xFF73D1FF);
    result.pallettes[2].keyword             = color_hexARGBTo01(0xFFFF9D00);
    result.pallettes[2].comment             = color_hexARGBTo01(0xFF5C6370);
    result.pallettes[2].preprocessor        = color_hexARGBTo01(0xFFC678DD);
    result.pallettes[2].string              = color_hexARGBTo01(0xFF98C379);

    // --- Evergreen (Earthy / Low Strain) ---
    result.pallettes[3].background          = color_hexARGBTo01(0xFF23272E);
    result.pallettes[3].backgroundVariation = color_hexARGBTo01(0xFF2C313A);
    result.pallettes[3].standard            = color_hexARGBTo01(0xFFDCD7BA);
    result.pallettes[3].variable            = color_hexARGBTo01(0xFF957FB8);
    result.pallettes[3].bracket             = color_hexARGBTo01(0xFFC0A36E);
    result.pallettes[3].function            = color_hexARGBTo01(0xFF7E9CD8);
    result.pallettes[3].keyword             = color_hexARGBTo01(0xFFFF5D62);
    result.pallettes[3].comment             = color_hexARGBTo01(0xFF727169);
    result.pallettes[3].preprocessor        = color_hexARGBTo01(0xFFE6C384);
    result.pallettes[3].string              = color_hexARGBTo01(0xFF98BB6C);

    // --- Paperback (Light Theme / Warm) ---
    result.pallettes[4].background          = color_hexARGBTo01(0xFFF5F2E9);
    result.pallettes[4].backgroundVariation = color_hexARGBTo01(0xFFE8E4D8);
    result.pallettes[4].standard            = color_hexARGBTo01(0xFF37352F);
    result.pallettes[4].variable            = color_hexARGBTo01(0xFFD91E18);
    result.pallettes[4].bracket             = color_hexARGBTo01(0xFF5B391E);
    result.pallettes[4].function            = color_hexARGBTo01(0xFF006699);
    result.pallettes[4].keyword             = color_hexARGBTo01(0xFFAD4B00);
    result.pallettes[4].comment             = color_hexARGBTo01(0xFF90908A);
    result.pallettes[4].preprocessor        = color_hexARGBTo01(0xFF7A3E9D);
    result.pallettes[4].string              = color_hexARGBTo01(0xFF448C27);

    // --- Cold Iron (Industrial / Blue-Grey) ---
    result.pallettes[5].background          = color_hexARGBTo01(0xFF1E2127);
    result.pallettes[5].backgroundVariation = color_hexARGBTo01(0xFF282C34);
    result.pallettes[5].standard            = color_hexARGBTo01(0xFFD1D1D1);
    result.pallettes[5].variable            = color_hexARGBTo01(0xFF61AFEF);
    result.pallettes[5].bracket             = color_hexARGBTo01(0xFFAFB6C3);
    result.pallettes[5].function            = color_hexARGBTo01(0xFF4DB5BD);
    result.pallettes[5].keyword             = color_hexARGBTo01(0xFF9FA7BA);
    result.pallettes[5].comment             = color_hexARGBTo01(0xFF545862);
    result.pallettes[5].preprocessor        = color_hexARGBTo01(0xFFC678DD);
    result.pallettes[5].string              = color_hexARGBTo01(0xFF56B6C2);


	return result;

}

enum EditorColorType {
	HANDMADE_HERO,
	THE_WITNESS
};



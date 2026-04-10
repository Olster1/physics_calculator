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
	Editor_Color_Palette witness;
	Editor_Color_Palette handmade;
	Editor_Color_Palette midnight;
    Editor_Color_Palette evergreen;
    Editor_Color_Palette paperback;
    Editor_Color_Palette cold_iron;
};

static Color_Palettes init_color_palettes() {

	Color_Palettes result = {};

	result.handmade.background = color_hexARGBTo01(0xFF161616);
	result.handmade.backgroundVariation = color_hexARGBTo01(0xFF3c3d3d);
	result.handmade.standard =  color_hexARGBTo01(0xFFA08563);
	result.handmade.variable = color_hexARGBTo01(0xFF6B8E23);
	result.handmade.bracket = color_hexARGBTo01(0xFFDAB98F);
	result.handmade.function = color_hexARGBTo01(0xFF008563);
	result.handmade.keyword = color_hexARGBTo01(0xFFCD950C);
	result.handmade.comment = color_hexARGBTo01(0xFF7D7D7D);
	result.handmade.preprocessor = color_hexARGBTo01(0xFFDAB98F);
	result.handmade.string = color_hexARGBTo01(0xFFDAB98F);


	result.witness.background = color_rgb255_to_01(6, 38, 38);
	result.witness.backgroundVariation = color_rgb255_to_01(6, 55, 55);
	result.witness.standard =  color_rgb255_to_01(194, 205, 187);
	result.witness.variable = color_rgb255_to_01(139, 194, 186);
	result.witness.bracket = color_rgb255_to_01(194, 205, 187);
	result.witness.function = color_rgb255_to_01(194, 205, 187);
	result.witness.keyword = color_rgb255_to_01(176, 200, 202);
	result.witness.comment = color_rgb255_to_01(92, 175, 87);
	result.witness.preprocessor = color_rgb255_to_01(118, 170, 138);
	result.witness.string = color_rgb255_to_01(118, 170, 138);

	// --- Mignight ---
	result.midnight.background          = color_hexARGBTo01(0xFF0B0E14);
    result.midnight.backgroundVariation = color_hexARGBTo01(0xFF1A1C23);
    result.midnight.standard            = color_hexARGBTo01(0xFFABB2BF);
    result.midnight.variable            = color_hexARGBTo01(0xFF00E5FF);
    result.midnight.bracket             = color_hexARGBTo01(0xFFF170FF);
    result.midnight.function            = color_hexARGBTo01(0xFF73D1FF);
    result.midnight.keyword             = color_hexARGBTo01(0xFFFF9D00);
    result.midnight.comment             = color_hexARGBTo01(0xFF5C6370);
    result.midnight.preprocessor        = color_hexARGBTo01(0xFFC678DD);
    result.midnight.string              = color_hexARGBTo01(0xFF98C379);

    // --- Evergreen (Earthy / Low Strain) ---
    result.evergreen.background          = color_hexARGBTo01(0xFF23272E);
    result.evergreen.backgroundVariation = color_hexARGBTo01(0xFF2C313A);
    result.evergreen.standard            = color_hexARGBTo01(0xFFDCD7BA);
    result.evergreen.variable            = color_hexARGBTo01(0xFF957FB8);
    result.evergreen.bracket             = color_hexARGBTo01(0xFFC0A36E);
    result.evergreen.function            = color_hexARGBTo01(0xFF7E9CD8);
    result.evergreen.keyword             = color_hexARGBTo01(0xFFFF5D62);
    result.evergreen.comment             = color_hexARGBTo01(0xFF727169);
    result.evergreen.preprocessor        = color_hexARGBTo01(0xFFE6C384);
    result.evergreen.string              = color_hexARGBTo01(0xFF98BB6C);

    // --- Paperback (Light Theme / Warm) ---
    result.paperback.background          = color_hexARGBTo01(0xFFF5F2E9);
    result.paperback.backgroundVariation = color_hexARGBTo01(0xFFE8E4D8);
    result.paperback.standard            = color_hexARGBTo01(0xFF37352F);
    result.paperback.variable            = color_hexARGBTo01(0xFFD91E18);
    result.paperback.bracket             = color_hexARGBTo01(0xFF5B391E);
    result.paperback.function            = color_hexARGBTo01(0xFF006699);
    result.paperback.keyword             = color_hexARGBTo01(0xFFAD4B00);
    result.paperback.comment             = color_hexARGBTo01(0xFF90908A);
    result.paperback.preprocessor        = color_hexARGBTo01(0xFF7A3E9D);
    result.paperback.string              = color_hexARGBTo01(0xFF448C27);

    // --- Cold Iron (Industrial / Blue-Grey) ---
    result.cold_iron.background          = color_hexARGBTo01(0xFF1E2127);
    result.cold_iron.backgroundVariation = color_hexARGBTo01(0xFF282C34);
    result.cold_iron.standard            = color_hexARGBTo01(0xFFD1D1D1);
    result.cold_iron.variable            = color_hexARGBTo01(0xFF61AFEF);
    result.cold_iron.bracket             = color_hexARGBTo01(0xFFAFB6C3);
    result.cold_iron.function            = color_hexARGBTo01(0xFF4DB5BD);
    result.cold_iron.keyword             = color_hexARGBTo01(0xFF9FA7BA);
    result.cold_iron.comment             = color_hexARGBTo01(0xFF545862);
    result.cold_iron.preprocessor        = color_hexARGBTo01(0xFFC678DD);
    result.cold_iron.string              = color_hexARGBTo01(0xFF56B6C2);


	return result;

}

enum EditorColorType {
	HANDMADE_HERO,
	THE_WITNESS
};



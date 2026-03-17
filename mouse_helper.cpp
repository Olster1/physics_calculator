float2 getWorldPFromMouse(GameState *gameState, float2 planeSize, bool useCameraOffset = false) {
    const float2 plane = scale_float2(0.5f, planeSize);
                
    const float x = lerp(-plane.x, plane.x, make_lerpTValue(gameState->mouseP_01.x));
    const float y = lerp(-plane.y, plane.y, make_lerpTValue(gameState->mouseP_01.y));

    float2 worldP = make_float2(x, y);

    if(useCameraOffset) {
        worldP.x -= gameState->camera.T.pos.x;
        worldP.y -= gameState->camera.T.pos.y;
    }

    return worldP;
}

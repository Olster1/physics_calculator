struct Camera {
    TransformX T;
    float2 targetP;
};

float2 camera_shakeOffset(float shakeTimer, float dt) {
    float2 p = make_float2(0, 0);
    float offset = SimplexNoise_fractal_1d(1, shakeTimer, 3);
    p.x = offset;
    p.y = offset;
    return p;
}

float16 makeWorldToCameraT(Camera *camera) {
    float16 T = getModelToViewSpaceWithoutScale(camera->T);
    return T;
}

void updateCamera(Camera *camera) {
    camera->T.pos.xy = camera->targetP;//lerp_float3(camera->T.pos, make_float3(camera->targetP.x, camera->targetP.y, camera->T.pos.z), 0.4f);
}

void camera_reset(Camera *camera) {
    easyPlatform_clearMemory(camera, sizeof(Camera));
}
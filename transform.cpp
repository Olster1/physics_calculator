struct TransformX {
    float3 pos;
    float3 scale;
    float4 rotation; //NOTE: Quarternion
};

struct SQT {
    float3 scale;
    float4 rotation;
    float3 pos;
};

SQT SQT_identity() {
    SQT result;
    result.scale = make_float3(1, 1, 1);
    result.pos = make_float3(0, 0, 0);
    result.rotation = identityQuaternion();
    return result;
}

TransformX make_transformX(float3 pos, float3 scale, float4 rotation = make_float4(0, 0, 0, 1)) {
    TransformX t = {};
    t.pos = pos;
    t.scale = scale;
    t.rotation = rotation;

    return t;
}

TransformX make_transformX_float2(float2 pos, float2 scale, float rotation = 0) {
    TransformX t = {};
    t.pos = make_float3(pos.x, pos.y, 1);
    t.scale = make_float3(scale.x, scale.y, 1);
    t.rotation = make_float4(rotation, 0, 0, 1);

    return t;
}

float16 getModelToViewSpace_euler(TransformX T) {
    float16 i = float16_identity();

    i = float16_scale(i, T.scale);
    i = float16_multiply(eulerAnglesToTransform(T.rotation.y, T.rotation.x, T.rotation.z), i);
    i = float16_set_pos(i, T.pos);

    return i;
}

float16 getModelToViewSpaceWithoutScale(TransformX T) {
    float16 i = float16_identity();

    i = float16_multiply(eulerAnglesToTransform(T.rotation.y, T.rotation.x, T.rotation.z), i);
    i = float16_set_pos(i, T.pos);

    return i;
}

float16 getModelToViewSpaceWithoutScaleSQT(TransformX T) {
    float16 i = quaternionToMatrix(T.rotation);
    i = float16_set_pos(i, T.pos);

    return i;
}

float16 getModelToViewSpace_extraRotationAboutZ(TransformX T, float rotation) {
    float16 i = float16_identity();

    i = float16_scale(i, T.scale);
    i = float16_multiply(eulerAnglesToTransform(T.rotation.y, T.rotation.x, T.rotation.z + rotation), i);
    i = float16_set_pos(i, T.pos);

    return i;
}

float16 transform_getInverseX(TransformX T) {
    float16 cameraT = float16_set_pos(float16_identity(), float3_negate(T.pos));
    float16 rotT = eulerAnglesToTransform(T.rotation.y, T.rotation.x, T.rotation.z);
    rotT = float16_transpose(rotT);
    cameraT = float16_multiply(rotT, cameraT);
    return cameraT;
}

float16 transform_getInverseX_SQT(TransformX T) {
    float16 cameraT = float16_set_pos(float16_identity(), float3_negate(T.pos));
    float16 rotT = quaternionToMatrix(T.rotation);
    rotT = float16_transpose(rotT);
    cameraT = float16_multiply(rotT, cameraT);
    return cameraT;
}

float16 getCameraX_withoutTranslation(TransformX T) {
    float16 rotT = eulerAnglesToTransform(T.rotation.y, T.rotation.x, T.rotation.z);
    rotT = float16_transpose(rotT);
    return rotT;
}
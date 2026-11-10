struct PSInput {
    float4 color : TEXCOORD0;
    float2 uv    : TEXCOORD1;
};

float4 main(PSInput input) : SV_Target0 {
    float d2 = dot(input.uv, input.uv);
    if (d2 > 1.0) discard;
    float a = 1.0 - d2;

    float4 color = float4(input.color.rgb * a, a);

    return color;
}
#include "particle.hlsli"

StructuredBuffer<Particle> Particles : register(t0, space0);

cbuffer ViewParams : register(b0, space1) {
    float2 aspectScale;   // (height/width, 1) so circles stay round
    float  particleSize;
    float  _pad0;
};

struct VSOutput {
    float4 color    : TEXCOORD0;
    float2 uv       : TEXCOORD1;
    float4 position : SV_Position;
};

static const float2 Corners[6] = {
    float2(-1, -1), float2( 1, -1), float2( 1,  1),
    float2(-1, -1), float2( 1,  1), float2(-1,  1),
};

VSOutput main(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID) {
    Particle p = Particles[instanceId];
    float2 corner = Corners[vertexId];

    VSOutput o;
    o.position = float4(p.position + corner * particleSize * aspectScale, 0.0, 1.0);
    o.uv       = corner;
    o.color    = p.color;
    return o;
}
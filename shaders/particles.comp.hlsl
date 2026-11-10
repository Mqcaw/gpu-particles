#include "particle.hlsli"

RWStructuredBuffer<Particle> Particles : register(u0, space1);

cbuffer SimParticles : register(b0, space2) {
    float deltaTime;
    float time;
    uint particleCount;
    float _pad0;
}

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint i = id.x;
    if (i >= particleCount) return;

    Particle p = Particles[i];

    p.velocity.y -= 0.5 * deltaTime;
    p.position   += p.velocity * deltaTime;

    if (abs(p.position.x) > 1.0) { p.position.x = clamp(p.position.x, -1.0, 1.0); p.velocity.x *= -0.9; }
    if (abs(p.position.y) > 1.0) { p.position.y = clamp(p.position.y, -1.0, 1.0); p.velocity.y *= -0.9; }

    Particles[i] = p;
}
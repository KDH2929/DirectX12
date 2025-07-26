
cbuffer CB_MVP : register(b0)
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4x4 modelInvTranspose;
};


cbuffer CB_CloudParameters : register(b1)
{
    // Local Space AABB Box
    float3 VolumeAABBMin; // Minimum corner of AABB
    float _pad0;
    float3 VolumeAABBMax; // Maximum corner of AABB
    float _pad1;

    float3 CameraPosition; // World-space camera position
    float StepSize;             // Ray marching step length
    float3 SunDirection;     // Normalized light direction
    float NoiseFrequency;   // Noise scale (frequency)

    float4x4 InverseViewProj; // (View*Proj)^{-1}
};


cbuffer CB_Global : register(b3)
{
    float Time;
    float3 _padG;
};
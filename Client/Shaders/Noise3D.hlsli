#ifndef NOISE3D_HLSLI
#define NOISE3D_HLSLI

// Source : https://github.com/stegu/webgl-noise/blob/master/src/noise3D.glsl

#define MOD289(x) (x - floor(x * (1.0 / 289.0)) * 289.0)
#define PERMUTE(x) MOD289(((x) * 34.0 + 1.0) * (x))
#define TAYLOR_INV_SQRT(x) (1.79284291400159 - 0.85373472095314 * x)

// 3D Gradient Noise (Perlin)
float PerlinNoise3D(float3 v)
{
    // 상수
    const float C1 = 1.0 / 6.0; // C.x
    const float C2 = 1.0 / 3.0; // C.y
    const float4 D = float4(0, 0.5, 1.0, 2.0);

    float3 i = floor(v + dot(v, float3(C2, C2, C2)));
    float3 x0 = v - i + dot(i, float3(C1, C1, C1));

    float3 g = step(x0.yzx, x0.xyz);
    float3 l = 1.0 - g;
    float3 i1 = min(g, l.zxy);
    float3 i2 = max(g, l.zxy);

    // x1, x2, x3
    float3 x1 = x0 - i1 + float3(C1, C1, C1);
    float3 x2 = x0 - i2 + float3(C2, C2, C2);
    float3 x3 = x0 - float3(D.y, D.y, D.y);

    // Permutation 계산
    i = MOD289(i);
    float4 p = PERMUTE(PERMUTE(PERMUTE(
                 i.z + float4(0, i1.z, i2.z, 1.0))
               + i.y + float4(0, i1.y, i2.y, 1.0))
               + i.x + float4(0, i1.x, i2.x, 1.0));

    // 그래디언트 인덱스
    float4 j = p - 49.0 * floor(p * (1.0 / 49.0));
    float4 x_ = floor(j * (1.0 / 7.0));
    float4 y_ = floor(j - 7.0 * x_);

    // x, y 좌표 계산 (float4 연산에 스칼라 사용)
    float4 x = x_ * C1 + C2;
    float4 y = y_ * C1 + C2;
    float4 h = 1.0 - abs(x) - abs(y);

    // 2D → 3D 그래디언트
    float4 b0 = float4(x.xy, y.xy);
    float4 b1 = float4(x.zw, y.zw);

    float4 s0 = floor(b0) * 2.0 + 1.0;
    float4 s1 = floor(b1) * 2.0 + 1.0;
    float4 sh = -step(h, 0.0);

    float4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    float4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

    // 실제 gradient 벡터들
    float3 g0 = float3(a0.xy, h.x);
    float3 g1 = float3(a0.zw, h.y);
    float3 g2 = float3(a1.xy, h.z);
    float3 g3 = float3(a1.zw, h.w);

    // 정규화
    float4 norm = TAYLOR_INV_SQRT(float4(
        dot(g0,g0), dot(g1,g1), dot(g2,g2), dot(g3,g3)));
    g0 *= norm.x;
    g1 *= norm.y;
    g2 *= norm.z;
    g3 *= norm.w;

    // 기여도 계산 및 합산
    float4 m = max(0.6 - float4(
        dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0);
    m = m * m;

    return 42.0 * dot(m * m, float4(
        dot(g0, x0), dot(g1, x1), dot(g2, x2), dot(g3, x3)));
}

#endif // NOISE3D_HLSLI
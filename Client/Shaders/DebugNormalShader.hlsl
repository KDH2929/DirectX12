cbuffer CB_MVP : register(b0)
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4x4 modelInvTranspose;
};

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT; 
    float2 uv : TEXCOORD; 
};

struct VSOutput
{
    float4 posClip : SV_POSITION;
    float3 worldPos : POSITION;
    float3 worldNormal : NORMAL;
};

// VS: 위치와 노말을 월드 공간으로 넘겨줌
VSOutput VSMain(VSInput IN)
{
    VSOutput OUT;
    float4 worldPos4 = mul(float4(IN.pos, 1.0), model);
    OUT.worldPos = worldPos4.xyz;
    OUT.worldNormal = normalize(mul(IN.normal, (float3x3)model));
    float4 viewPos4 = mul(worldPos4, view);
    OUT.posClip = mul(viewPos4, projection);
    return OUT;
}

// GS 입력/출력 (VSOutput 그대로 사용)
struct GSOutput
{
    float4 posClip : SV_POSITION;
    float4 color : COLOR0;
};

[maxvertexcount(2)]     // 한 점당 최대 두 개의 정점으로 변환
void GSMain(point VSOutput IN[1], inout LineStream<GSOutput> stream)
{
    GSOutput o;

    // 기존 정점 한 개를 정점 2개로 바꾸어 선분으로 만들어줌
    // stream 에 넣은 정점들이 새로운 Mesh를 만드는 것

    // 시작점: 버텍스 위치
    o.posClip = IN[0].posClip;
    o.color = float4(1, 0, 0, 1);  // 빨간색
    stream.Append(o);

    // 끝점: 노말 방향으로 이동
    float3 endWS = IN[0].worldPos + normalize(IN[0].worldNormal) * 0.3;
    float4 endView = mul(float4(endWS, 1.0), view);
    o.posClip = mul(endView, projection);
    o.color = float4(1, 0, 0, 1);
    stream.Append(o);

    // 선분 한 개를 다 그렸고 다음 선분을 그리기 위해 Restart
    stream.RestartStrip();
}

float4 PSMain(GSOutput IN) : SV_Target
{
    return IN.color;
}

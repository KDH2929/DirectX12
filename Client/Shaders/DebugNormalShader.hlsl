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

// VS: ��ġ�� �븻�� ���� �������� �Ѱ���
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

// GS �Է�/��� (VSOutput �״�� ���)
struct GSOutput
{
    float4 posClip : SV_POSITION;
    float4 color : COLOR0;
};

[maxvertexcount(2)]     // �� ���� �ִ� �� ���� �������� ��ȯ
void GSMain(point VSOutput IN[1], inout LineStream<GSOutput> stream)
{
    GSOutput o;

    // ���� ���� �� ���� ���� 2���� �ٲپ� �������� �������
    // stream �� ���� �������� ���ο� Mesh�� ����� ��

    // ������: ���ؽ� ��ġ
    o.posClip = IN[0].posClip;
    o.color = float4(1, 0, 0, 1);  // ������
    stream.Append(o);

    // ����: �븻 �������� �̵�
    float3 endWS = IN[0].worldPos + normalize(IN[0].worldNormal) * 0.3;
    float4 endView = mul(float4(endWS, 1.0), view);
    o.posClip = mul(endView, projection);
    o.color = float4(1, 0, 0, 1);
    stream.Append(o);

    // ���� �� ���� �� �׷Ȱ� ���� ������ �׸��� ���� Restart
    stream.RestartStrip();
}

float4 PSMain(GSOutput IN) : SV_Target
{
    return IN.color;
}

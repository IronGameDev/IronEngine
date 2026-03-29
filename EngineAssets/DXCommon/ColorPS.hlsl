
cbuffer PushConstants : register(b0)
{
    float4 Color;
}

struct VSOutput {
    noperspective float4 Position : SV_Position;
    noperspective float2 UV : TEXCOORD;
};

float4 ColorPS(in VSOutput psIn) : SV_Target0 {
    return Color;
}

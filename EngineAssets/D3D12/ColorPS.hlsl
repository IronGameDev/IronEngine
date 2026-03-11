
struct VSOutput {
    noperspective float4 Position : SV_Position;
    noperspective float2 UV : TEXCOORD;
};

float4 ColorPS(in VSOutput psIn) : SV_Target0 {
    return float4(1.f, 0.f, 1.f, 1.f);
}

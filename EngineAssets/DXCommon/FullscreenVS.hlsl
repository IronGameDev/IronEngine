
struct VSOutput
{
    noperspective float4 Position : SV_Position;
    noperspective float2 UV : TEXCOORD;
};

//VSOutput FullScreenVS(uint VertexIdx : SV_VertexID) {
//    VSOutput o;

//    float2 pos[3] = {
//        float2(-1.0, -1.0),
//        float2(-1.0,  3.0),
//        float2( 3.0, -1.0)
//    };

//    float2 uv[3] = {
//        float2(0.0, 1.0),
//        float2(0.0, -1.0),
//        float2(2.0, 1.0)
//    };

//    o.Position = float4(pos[VertexIdx], 0.0, 1.0);
//    o.UV = uv[VertexIdx];

//    return o;
//}

VSOutput FullScreenVS(in float3 pos : POS, uint VertexIdx : SV_VertexID) {
    VSOutput o;

    float2 uv[3] = {
        float2(0.0, 1.0),
        float2(0.0, -1.0),
        float2(2.0, 1.0)
    };

    o.Position = float4(pos.xyz, 1.0);
    o.UV = uv[VertexIdx];

    return o;
}
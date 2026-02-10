static float2 position[3] = float2[](
    float2( 0.0, -0.5),
    float2( 0.5,  0.5),
    float2(-0.5,  0.5)
);

static float3 color[3] = float3[](
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0)
);

struct Vertex {
    float4 pos : SV_Position;
    float3 col;
};

[shader("vertex")]
Vertex vertMain(uint id: SV_VertexID) {
    Vertex output;
    output.pos = float4(position[id], 0.0, 0.1);
    output.col = color[id];
    return output;
}

[shader("fragment")]
float4 fragMain(Vertex input): SV_Target {
    float3 color = input.col;
    return float4(color, 1.0);
}

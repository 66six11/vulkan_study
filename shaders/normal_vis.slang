// Normal Visualization Shader
// Displays vertex normals as RGB colors

struct VertexInput
{
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 normal : NORMAL;
    [[vk::location(2)]] float2 uv : TEXCOORD;
    [[vk::location(3)]] float3 color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
};

// Push constants for MVP matrix
struct PushConsts
{
    float4x4 mvp;
};
[[vk::push_constant]] PushConsts pushConsts;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(pushConsts.mvp, float4(input.position, 1.0));
    output.normal = input.normal;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    // Visualize normal as color (normal is in [-1, 1], map to [0, 1])
    float3 normal_color = input.normal * 0.5 + 0.5;
    return float4(normal_color, 1.0);
}

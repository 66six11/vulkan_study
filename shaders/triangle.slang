// Triangle Shader - Slang
// Compiles to SPIR-V for Vulkan

// Vertex input/output
struct VertexInput
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

// Push constants for MVP matrix (与 PBR 着色器保持一致)
struct PushConsts
{
    float4x4 mvp;
};
[[vk::push_constant]] PushConsts pushConsts;

// Vertex Shader
[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(pushConsts.mvp, float4(input.position, 1.0));
    output.color = float4(input.color, 1.0);
    return output;
}

// Fragment Shader
[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return input.color;
}
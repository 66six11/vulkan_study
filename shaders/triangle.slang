// Triangle Shader - Slang
// Compiles to SPIR-V for Vulkan

// Uniform buffer with MVP matrix
struct UniformBufferObject
{
    float4x4 mvp;
};

[[vk::binding(0, 0)]]
ConstantBuffer<UniformBufferObject> ubo;

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

// Vertex Shader
[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(ubo.mvp, float4(input.position, 1.0));
    output.color = float4(input.color, 1.0);
    return output;
}

// Fragment Shader
[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return input.color;
}
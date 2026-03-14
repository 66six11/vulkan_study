// PBR Shader with material support
// Uses push constants for MVP matrix and uniform buffer for material params

struct MaterialParams
{
    float4 color;
    float roughness;
    float metallic;
    float emissive;
    float padding;
};

[[vk::binding(0, 0)]]
ConstantBuffer<MaterialParams> material;

struct VertexInput
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 world_pos : TEXCOORD0;
    float3 color : TEXCOORD1;
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
    output.world_pos = input.position;
    output.color = input.color;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    // Apply material color (ignore vertex color for clearer material visualization)
    float3 base_color = material.color.rgb;
    
    // Metallic: blend toward white/silver
    float3 metallic_color = lerp(base_color, float3(0.9, 0.95, 1.0), material.metallic);
    
    // Emissive: add glow
    float3 emissive_contrib = base_color * material.emissive * 2.0;
    
    // Combine
    float3 final_color = metallic_color + emissive_contrib;
    
    // Roughness affects brightness slightly
    final_color *= (1.0 - material.roughness * 0.3);
    
    return float4(final_color, 1.0);
}
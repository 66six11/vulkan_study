// PBR Shader with material support
// Uses push constants for MVP matrix and uniform buffer for material params

struct MaterialParams
{
    float4 color;
    float roughness;
    float metallic;
    float emissive;
    float has_texture;  // 1.0 if texture is present, 0.0 otherwise
};

[[vk::binding(0, 0)]]
ConstantBuffer<MaterialParams> material;

[[vk::binding(1, 0)]]
Texture2D albedo_texture;

[[vk::binding(2, 0)]]
SamplerState texture_sampler;

struct VertexInput
{
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 color : COLOR;
    [[vk::location(2)]] float2 uv : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 world_pos : TEXCOORD0;
    float3 color : TEXCOORD1;
    float2 uv : TEXCOORD2;
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
    output.uv = input.uv;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    // Sample texture if available
    float3 texture_color = albedo_texture.Sample(texture_sampler, input.uv).rgb;
    
    // Mix between material color and texture based on has_texture flag
    float use_texture = material.has_texture > 0.5 ? 1.0 : 0.0;
    float3 base_color = lerp(material.color.rgb, texture_color * material.color.rgb, use_texture);
    
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
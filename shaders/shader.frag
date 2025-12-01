#version 450

layout (location = 0) in vec4 inColor;     // 对应顶点着色器的 layout(location=0) out
layout (location = 0) out vec4 outColor;

void main()
{
    // 直接输出插值后的顶点颜色
    outColor = inColor;
}
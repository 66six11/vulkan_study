#version 450
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inColor;

layout (location = 0) out vec4 outColor;   // 传到片元的颜色

void main()
{
    // 直接使用顶点缓冲里的位置（假定已经是 NDC 或者你后面会加 MVP 矩阵）
    gl_Position = vec4(inPosition, 1.0);

    // 把顶点颜色传给片元着色器
    outColor = inColor;
}
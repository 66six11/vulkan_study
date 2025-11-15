#version 450

// 片段着色器：简单的红色片段着色器

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0); // 红色
}
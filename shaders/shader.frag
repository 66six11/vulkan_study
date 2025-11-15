#version 450

/**
 * @brief 片段着色器：简单的红色片段着色器
 * 
 * 该着色器将所有片段渲染为红色，不进行任何复杂的光照计算
 */

// 定义输出颜色变量，location=0对应于渲染通道中第一个颜色附件
layout(location = 0) out vec4 outColor;

void main() {
    // 将片段颜色设置为红色 (R=1.0, G=0.0, B=0.0, A=1.0)
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
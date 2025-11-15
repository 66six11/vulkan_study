#version 450

// 顶点着色器：简单的三角形顶点着色器

void main() {
    // 三角形的三个顶点 - 在标准化设备坐标中
    vec2 positions[3] = vec2[](
        vec2(0.0, -0.5),   // 底部中心
        vec2(0.5, 0.5),    // 右上角
        vec2(-0.5, 0.5)    // 左上角
    );

    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
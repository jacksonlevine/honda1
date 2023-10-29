#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 uv;
out vec3 vertexColor;
out vec2 TexCoord;
uniform mat4 mvp;
void main()
{
    gl_Position = mvp * vec4(position, 1.0);
    vertexColor = color;
    TexCoord = uv;
}
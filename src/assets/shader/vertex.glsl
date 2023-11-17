#version 450 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
out vec3 vertexColor;
out vec2 TexCoord;
out vec3 pos;
uniform mat4 mvp;
void main()
{
    gl_Position = mvp * vec4(position, 1.0);
    vertexColor = vec3(1.0, 1.0, 1.0);
    TexCoord = uv;
    pos = position;
}
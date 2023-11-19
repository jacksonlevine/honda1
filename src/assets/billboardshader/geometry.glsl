#version 450 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in vec2 TexCoord[];
in vec3 vertexColor[];
in vec3 pos[];

uniform mat4 p;
uniform mat4 v;
uniform vec3 camPos;

out vec2 outTexCoord;  // Corrected: Output UV coordinates to fragment shader
out vec3 outVertexColor; // Corrected: Output vertex color to fragment shader
out vec3 outPos;       // Corrected: Output position to fragment shader

void main() {

    float textureWidth = 0.0588235294117647;

    float billboardSize = 3.0; // Size of the billboard

    vec3 toCamera = normalize(camPos - pos[0]);
    vec3 upVector = vec3(0.0, 1.0, 0.0); // World up vector
    vec3 rightVector = normalize(cross(upVector, toCamera));

    vec3 billboardVertex1 = pos[0] - rightVector * billboardSize + upVector * billboardSize;
    vec3 billboardVertex2 = pos[0] + rightVector * billboardSize + upVector * billboardSize;
    vec3 billboardVertex3 = pos[0] - rightVector * billboardSize - upVector * billboardSize;
    vec3 billboardVertex4 = pos[0] + rightVector * billboardSize - upVector * billboardSize;

    outTexCoord = TexCoord[0];
    outVertexColor = vertexColor[0];
    outPos = billboardVertex1;
    gl_Position = p * v * vec4(billboardVertex1, 1.0);
    EmitVertex();

    outTexCoord = vec2(TexCoord[0].x, TexCoord[0].y + textureWidth);
    outVertexColor = vertexColor[0];
    outPos = billboardVertex2;
    gl_Position = p * v * vec4(billboardVertex2, 1.0);
    EmitVertex();

    outTexCoord = vec2(TexCoord[0].x - textureWidth, TexCoord[0].y);
    outVertexColor = vertexColor[0];
    outPos = billboardVertex3;
    gl_Position = p * v * vec4(billboardVertex3, 1.0);
    EmitVertex();

    outTexCoord = vec2(TexCoord[0].x - textureWidth, TexCoord[0].y + textureWidth);
    outVertexColor = vertexColor[0];
    outPos = billboardVertex4;
    gl_Position = p * v * vec4(billboardVertex4, 1.0);
    EmitVertex();

    EndPrimitive();
}

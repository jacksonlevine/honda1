#version 450 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 4) out;

uniform mat4 p;
uniform mat4 v;

void main() {
    for (int i = 0; i < gl_in.length(); ++i) {
        vec4 position = view * gl_in[i].gl_Position;

        vec3 right = vec3(view[0][0], view[1][0], view[2][0]);
        vec3 up = vec3(view[0][1], view[1][1], view[2][1]);

        gl_Position = projection * (position - right + up);
        EmitVertex();

        gl_Position = projection * (position + right + up);
        EmitVertex();

        gl_Position = projection * (position - right - up);
        EmitVertex();

        gl_Position = projection * (position + right - up);
        EmitVertex();
    }
    EndPrimitive();
}
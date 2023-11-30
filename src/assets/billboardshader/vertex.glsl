#version 450 core


layout(location = 0) in vec3 vertexPosition; // Quad vertex positions
layout(location = 1) in vec3 instancePosition; // Instance position
layout(location = 2) in vec2 instanceUV0;  // First pair of UVs
layout(location = 3) in vec2 instanceUV1;  // Second pair of UVs
layout(location = 4) in vec2 instanceUV2;  // Third pair of UVs
layout(location = 5) in vec2 instanceUV3;  // Fourth pair of UVs
layout(location = 6) in float cornerID;    // Corner ID

out vec3 vertexColor;
out vec2 TexCoord;
out vec3 pos;

uniform mat4 v;
uniform mat4 p;
uniform mat4 m;
uniform vec3 camPos;

void main() {
    // Calculate the billboard orientation
    vec3 look = normalize(instancePosition - camPos); // Direction from camera to quad
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), look)); // Right vector
    vec3 up = cross(look, right); // Up vector

    // Apply billboard transformation
    vec3 billboardedPosition = instancePosition + (vertexPosition.x * right + vertexPosition.y * up);

    // Transform position to clip space
    gl_Position = p * v * m * vec4(billboardedPosition, 1.0);
    gl_Position.y -= pow(distance(camPos, instancePosition)*0.02, 3);

    // Selecting UV based on cornerID
    if (cornerID == 0.0) {
        TexCoord = instanceUV0;
    } else if (cornerID == 1.0) {
        TexCoord = instanceUV1;
    } else if (cornerID == 2.0) {
        TexCoord = instanceUV2;
    } else if (cornerID == 3.0) {
        TexCoord = instanceUV3;
    }

    vertexColor = vec3(1.0, 1.0, 1.0);
    pos = instancePosition;
}
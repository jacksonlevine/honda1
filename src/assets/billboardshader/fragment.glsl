#version 450 core
in vec3 outVertexColor;  // Updated to match the output from geometry shader
in vec2 outTexCoord;     // Updated to match the output from geometry shader
in vec3 outPos;          // Updated to match the output from geometry shader
out vec4 FragColor;
uniform sampler2D ourTexture;
uniform vec3 camPos;
uniform float brightness;

void main()
{
    vec4 texColor = texture(ourTexture, outTexCoord);

    if(texColor.a < 0.1) {
        discard;
    }

    vec3 fogColor = vec3(0.2, 0.2, 0.7);
    float diss = pow(distance(outPos, camPos)/35.0f, 2);

    if(diss < 0.9990f) {
        diss = 0; 
    } else {
        diss = (diss - 0.9990f); 
    }

    vec3 finalColor = mix(outVertexColor, fogColor, max(diss/16.0, 0));
    FragColor = mix(vec4(finalColor, 1.0) * texColor, vec4(fogColor, 1.0), max(diss/16.0, 0));
    FragColor.a = diss;

    if(FragColor.a < 0.3) {
        discard;
    } else {
        FragColor.a = 1.0;
    }
    FragColor = FragColor * brightness;
}

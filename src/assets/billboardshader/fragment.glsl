#version 450 core
in vec3 vertexColor;
in vec2 TexCoord;
in vec3 pos;
out vec4 FragColor;
uniform sampler2D ourTexture;
uniform vec3 camPos;
uniform float brightness;
void main()
{

    vec4 texColor = texture(ourTexture, TexCoord);

    if(texColor.a < 0.1) {
        discard;
    }

    vec3 fogColor = vec3(0.2, 0.2, 0.7);
    float diss = pow(distance(pos, camPos)/35.0f, 2);

    if(diss < 0.9990f) {
        diss = 0; 
    } else {
        diss = (diss-0.9990f); 
    }

    vec3 finalColor = mix(vertexColor, fogColor, max(diss/16.0, 0));
    FragColor = mix(vec4(finalColor, 1.0) * texColor, vec4(fogColor, 1.0), max(diss/16.0, 0));
    FragColor.a = diss;

    if(FragColor.a < 0.3) {
        discard;
    } else {
        FragColor.a = 1.0;
    }
    FragColor = FragColor * brightness;
}
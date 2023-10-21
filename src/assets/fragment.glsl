#version 450 core
in vec3 vertexColor;
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D ourTexture;
uniform vec3 camPos;
uniform int underWater;
void main()
{

    vec4 texColor = texture(ourTexture, TexCoord);

    if(texColor.a < 0.1) {
        discard;
    }

    vec3 fogColor = vec3(0.2, 0.2, 0.7);
    float diss = pow(gl_FragCoord.z, 2);

    if(underWater == 0) {
        if(gl_FragCoord.z < 0.9978f) { 
            diss = 0; 
        } else { 
            diss = (diss-0.9978f)*1000; 
        }
    } else { 
        diss = (diss-0.6)*4; 
    }

    vec3 finalColor = mix(vertexColor, fogColor, max(diss/4.0, 0));
    FragColor = mix(vec4(finalColor, 1.0) * texColor, vec4(fogColor, 1.0), max(diss/4.0, 0));
}
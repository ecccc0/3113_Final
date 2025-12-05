#version 330

uniform sampler2D texture0;
uniform vec2 lightPosition;
uniform int status; 

// Input from Vertex Shader
in vec2 fragTexCoord;
in vec2 fragPosition;
in vec4 fragColor;  

out vec4 finalColor;

// Lighting Constants
const float LINEAR_TERM    = 0.00003;
const float QUADRATIC_TERM = 0.00003;
const float MIN_BRIGHTNESS = 0.05;

float attenuate(float distance, float linearTerm, float quadraticTerm)
{
    float attenuation = 1.0 / (1.0 + linearTerm * distance + quadraticTerm * distance * distance);
    return max(attenuation, MIN_BRIGHTNESS);
}

void main()
{
    // Calculate Lighting
    float dist = distance(lightPosition, fragPosition);
    float brightness = attenuate(dist, LINEAR_TERM, QUADRATIC_TERM);

    // Fetch Texture Color AND Multiply by Vertex Color
    // This restores the Red/Transparent tints
    vec4 texColor = texture(texture0, fragTexCoord) * fragColor; 
    
    // Apply Lighting (Keep alpha intact)
    vec4 litColor = vec4(texColor.rgb * brightness, texColor.a);

    // Apply Status Logic (Spotted/Hidden)
    if (status == 1) // SPOTTED: Red Tint
    {
        // blend RGB toward red by 30%, preserve alpha
        float tint = 0.3;
        vec3 spottedRGB = litColor.rgb * (1.0 - tint) + vec3(1.0, 0.0, 0.0) * tint;
        finalColor = vec4(spottedRGB, litColor.a);
    }
    else if (status == 2) // HIDDEN: Darken
    {
        finalColor = vec4(litColor.rgb * 0.5, litColor.a);
    }
    else 
    {
        finalColor = litColor;
    }
}
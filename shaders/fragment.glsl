#version 330

uniform sampler2D texture0;
uniform vec2 lightPosition;
uniform int status; 

// Input from Vertex Shader
in vec2 fragTexCoord;
in vec2 fragPosition;
in vec4 fragColor;      // <--- NEW: Receive the color

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
    // 1. Calculate Lighting
    float dist = distance(lightPosition, fragPosition);
    float brightness = attenuate(dist, LINEAR_TERM, QUADRATIC_TERM);

    // 2. Fetch Texture Color AND Multiply by Vertex Color
    // This restores the Red/Transparent tints from your code
    vec4 texColor = texture(texture0, fragTexCoord) * fragColor; 
    
    // 3. Apply Lighting (Keep alpha intact)
    vec4 litColor = vec4(texColor.rgb * brightness, texColor.a);

    // 4. Apply Status Logic (Spotted/Hidden)
    if (status == 1) // SPOTTED: Red Tint
    {
        // Mix with pure red, preserving original alpha
        finalColor = mix(litColor, vec4(1.0, 0.0, 0.0, litColor.a), 0.3); 
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
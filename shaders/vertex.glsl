#version 330

// Input attributes from Raylib
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;    // <--- NEW: Accept the color from Draw functions

// Outputs to the Fragment Shader
out vec2 fragTexCoord;
out vec2 fragPosition;
out vec4 fragColor;     // <--- NEW: Pass the color along

uniform mat4 mvp;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor; // <--- NEW: Assign input color to output
    
    // Pass raw 2D position for your lighting logic
    fragPosition = vertexPosition.xy;
    
    // Calculate final vertex position
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
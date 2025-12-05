#version 330

// Input attributes from Raylib
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;  

// Outputs to the Fragment Shader
out vec2 fragTexCoord;
out vec2 fragPosition;
out vec4 fragColor;   

uniform mat4 mvp;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    
    // Pass raw 2D position for your lighting logic
    fragPosition = vertexPosition.xy;
    
    // Calculate final vertex position
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
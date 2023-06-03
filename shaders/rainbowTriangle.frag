#version 450

// vertex shader input
layout (location = 0) in vec3 inColour;

// output write
layout (location = 0) out vec4 outFragColour;

void main()
{
    // return red
    outFragColour = vec4(inColour, 1.0f);
}
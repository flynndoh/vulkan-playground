#version 450

// output write
layout (location = 0) out vec4 outFragColour;

void main()
{
    // return red
    outFragColour = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}
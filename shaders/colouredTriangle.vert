#version 450

//output variable to the fragment shader
layout (location = 0) out vec3 outColor;

void main()
{
    // const array of positions for the triangle
    const vec3 positions[3] = vec3[3](
    vec3(1.0f, 1.0f, 0.0f),
    vec3(-1.0f, 1.0f, 0.0f),
    vec3(0.0f, -1.0f, 0.0f)
    );

    // const array of positions for the triangle
    const vec3 colours[3] = vec3[3](
    vec3(1.0f, 0.0f, 0.0f), // red
    vec3(0.0f, 1.0f, 0.0f), // green
    vec3(0.0f, 0.0f, 1.0f)// blue
    );

    // output the position of each vertex
    gl_Position = vec4(positions[gl_VertexIndex], 1.0f);

    // update out colour
    outColor = colours[gl_VertexIndex];
}
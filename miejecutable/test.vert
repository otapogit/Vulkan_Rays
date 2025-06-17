#version 460

struct VertexData{
	float x,y,z;
	float u,v;
};

//layout (binding = 0) readonly buffer Vertices { VertexData data[]; } in_Vertices;
//en opengl gl_VertexId
//VertexData vtx = in_Vertices.data[gl_VertexIndex];

// Atributos de entrada desde vertex buffer
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;


//bindingpoint de este es uniform
//Define struct UniformBuffer con las cosas que tenga
//no hay nada de getuniformlocation sino uniformblocks
layout (binding = 1) readonly uniform UniformBuffer { mat4 MVP; } ubo;

layout(location = 0) out vec2 fragTexCoord;

void main(){

    gl_Position = ubo.MVP * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}
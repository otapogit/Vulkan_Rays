#version 460

/*
//in layout(location = 1) vec4 vPosition;
vec2 pos[3] = vec2[3]( vec2(0.7, -0.7), vec2(0.7, 0.7),  vec2(-0.7, 0.7)); 
vec2 pose[3] = vec2[3]( vec2(0.7, -0.7),vec2(-0.7, 0.7),vec2(-0.7, -0.7) );
void main() 
{
	if (gl_InstanceIndex == 0){
	gl_Position = vec4( pos[gl_VertexIndex], 0.0, 1.0 );
	}else if (gl_InstanceIndex == 1){
	gl_Position = vec4( pose[gl_VertexIndex], 0.0, 1.0 );
	}

}*/

struct VertexData{
	float x,y,z;
	float u,v;
};

layout (binding = 0) readonly buffer Vertices { VertexData data[]; } in_Vertices;
//bindingpoint de este es uniform
//Define struct UniformBuffer con las cosas que tenga
//no hay nada de getuniformlocation sino uniformblocks
layout (binding = 1) readonly uniform UniformBuffer { mat4 MVP; } ubo;

void main(){
	//en opengl gl_VertexId
	VertexData vtx = in_Vertices.data[gl_VertexIndex];
	vec3 pos = vec3(vtx.x, vtx.y, vtx.z);

	gl_Position = ubo.MVP * vec4(pos,1.0);
}
// ================================
// raytrace.rchit - Closest Hit Shader
// ================================
#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec3 attribs;

void main() {
    // Coordenadas barycéntricas del hit
    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    
    // Color basado en coordenadas barycéntricas (para visualización)
    hitValue = barycentricCoords;
}
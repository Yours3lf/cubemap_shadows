#version 430 core

uniform mat4 mv, p;
uniform mat4 normal_mat;

layout(location=0) in vec4 in_vertex;
layout(location=2) in vec3 in_normal;

out vec4 pos;
out vec4 vs_pos;
out vec3 vs_normal;

void main()
{
  vs_normal = (normal_mat * vec4( in_normal, 0 )).xyz;
  pos = in_vertex;
  vs_pos = mv * in_vertex;
  gl_Position = p * vs_pos;
}

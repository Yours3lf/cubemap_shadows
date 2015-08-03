#version 430 core

//get me a triangle
layout(triangles) in; 
//3 vertices per triangle, 6 triangles overall
layout(triangle_strip, max_vertices=18) out;

//per face view projection matrices
uniform mat4 cube_viewproj[6];

void main()
{
  //redirect to 6 cubemap faces
  for(gl_Layer = 0; gl_Layer < 6; ++gl_Layer)
  {
    for(int c = 0; c < 3; ++c)
    {
      gl_Position = cube_viewproj[gl_Layer] * gl_in[c].gl_Position;
      EmitVertex();
    }
    
    EndPrimitive();
  }
}

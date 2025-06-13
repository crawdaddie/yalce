#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
out vec3 vertexColor;
out vec3 worldPos;
void main() {
  vec4 worldPosition = uModel * vec4(aPos, 1.0);
  worldPos = worldPosition.xyz;
  
  vec4 viewProj = uProjection * uView * worldPosition;
  gl_Position = viewProj;
  vertexColor = aColor;
}


#version 330 core
in vec3 vertexColor;
in vec3 worldPos;
out vec4 FragColor;
void main() {
  vec2 coord = gl_PointCoord - vec2(0.5);
  float dist = length(coord);
  
  if (dist > 0.5) {
    discard;
  }
  
  float alpha = 1.0 - smoothstep(0.1, 0.5, dist);
  FragColor = vec4(vertexColor, alpha);
}

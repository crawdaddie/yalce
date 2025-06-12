#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Matrix math utilities
typedef struct {
  float m[16]; // Column-major order
} Mat4;

typedef struct {
  float x, y, z;
} Vec3;

void mat4_identity(Mat4 *m) {
  memset(m->m, 0, sizeof(float) * 16);
  m->m[0] = m->m[5] = m->m[10] = m->m[15] = 1.0f;
}

void mat4_perspective(Mat4 *m, float fov, float aspect, float near, float far) {
  mat4_identity(m);
  float f = 1.0f / tanf(fov * 0.5f);
  m->m[0] = f / aspect;
  m->m[5] = f;
  m->m[10] = (far + near) / (near - far);
  m->m[11] = -1.0f;
  m->m[14] = (2.0f * far * near) / (near - far);
  m->m[15] = 0.0f;
}

void mat4_lookat(Mat4 *m, Vec3 eye, Vec3 center, Vec3 up) {
  Vec3 f = {center.x - eye.x, center.y - eye.y, center.z - eye.z};
  float len = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
  f.x /= len;
  f.y /= len;
  f.z /= len;

  Vec3 s = {f.y * up.z - f.z * up.y, f.z * up.x - f.x * up.z,
            f.x * up.y - f.y * up.x};
  len = sqrtf(s.x * s.x + s.y * s.y + s.z * s.z);
  s.x /= len;
  s.y /= len;
  s.z /= len;

  Vec3 u = {s.y * f.z - s.z * f.y, s.z * f.x - s.x * f.z,
            s.x * f.y - s.y * f.x};

  mat4_identity(m);
  m->m[0] = s.x;
  m->m[4] = s.y;
  m->m[8] = s.z;
  m->m[12] = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
  m->m[1] = u.x;
  m->m[5] = u.y;
  m->m[9] = u.z;
  m->m[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
  m->m[2] = -f.x;
  m->m[6] = -f.y;
  m->m[10] = -f.z;
  m->m[14] = f.x * eye.x + f.y * eye.y + f.z * eye.z;
}

void mat4_multiply(Mat4 *result, const Mat4 *a, const Mat4 *b) {
  Mat4 temp;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      temp.m[i * 4 + j] = 0.0f;
      for (int k = 0; k < 4; k++) {
        temp.m[i * 4 + j] += a->m[i * 4 + k] * b->m[k * 4 + j];
      }
    }
  }
  *result = temp;
}

// Shader source code
const char *vertex_shader_source =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aColor;\n"
    "\n"
    "uniform mat4 uModel;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uProjection;\n"
    "\n"
    "out vec3 vertexColor;\n"
    "out vec3 worldPos;\n"
    "out vec3 viewPos;\n"
    "out vec4 clipPos;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    // Transform vertex through the pipeline\n"
    "    vec4 worldPosition = uModel * vec4(aPos, 1.0);\n"
    "    vec4 viewPosition = uView * worldPosition;\n"
    "    vec4 clipPosition = uProjection * viewPosition;\n"
    "    \n"
    "    // Pass data to fragment shader\n"
    "    worldPos = worldPosition.xyz;\n"
    "    viewPos = viewPosition.xyz;\n"
    "    clipPos = clipPosition;\n"
    "    vertexColor = aColor;\n"
    "    \n"
    "    // Final position for rasterization\n"
    "    gl_Position = clipPosition;\n"
    "}\0";

const char *fragment_shader_source =
    "#version 330 core\n"
    "in vec3 vertexColor;\n"
    "in vec3 worldPos;\n"
    "in vec3 viewPos;\n"
    "in vec4 clipPos;\n"
    "\n"
    "out vec4 FragColor;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    // Color based on depth to visualize 3D effect\n"
    "    float depth = -viewPos.z; // Distance from camera\n"
    "    float depthFactor = 1.0 - (depth / 10.0); // Fade with distance\n"
    "    depthFactor = clamp(depthFactor, 0.3, 1.0);\n"
    "    \n"
    "    // Combine vertex color with depth-based shading\n"
    "    vec3 finalColor = vertexColor * depthFactor;\n"
    "    \n"
    "    FragColor = vec4(finalColor, 1.0);\n"
    "}\0";

// Shader compilation helper
GLuint compile_shader(GLenum type, const char *source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    printf("Shader compilation failed: %s\n", infoLog);
    return 0;
  }
  return shader;
}

// Error callback for GLFW
void error_callback(int error, const char *description) {
  printf("GLFW Error %d: %s\n", error, description);
}

// Window resize callback
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

int main() {
  // Initialize GLFW
  glfwSetErrorCallback(error_callback);
  if (!glfwInit()) {
    printf("Failed to initialize GLFW\n");
    return -1;
  }

  // Configure GLFW
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create window
  GLFWwindow *window =
      glfwCreateWindow(800, 600, "3D Transformation Pipeline Demo", NULL, NULL);
  if (!window) {
    printf("Failed to create GLFW window\n");
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // Initialize GLEW
  if (glewInit() != GLEW_OK) {
    printf("Failed to initialize GLEW\n");
    return -1;
  }

  printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
  printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

  // Compile shaders
  GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
  GLuint fragment_shader =
      compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
  if (!vertex_shader || !fragment_shader) {
    return -1;
  }

  // Create shader program
  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);

  GLint success;
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
    printf("Shader program linking failed: %s\n", infoLog);
    return -1;
  }

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  // Triangle vertices with colors (position + color)
  float vertices[] = {
      // Positions        // Colors
      0.0f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, // Top vertex - Red
      -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // Bottom left - Green
      0.5f,  -0.5f, 0.0f, 0.0f, 0.0f, 1.0f  // Bottom right - Blue
  };

  // Create and bind Vertex Array Object
  GLuint VAO, VBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Position attribute (location = 0)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Color attribute (location = 1)
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Enable depth testing
  glEnable(GL_DEPTH_TEST);

  // Get uniform locations
  GLint model_loc = glGetUniformLocation(shader_program, "uModel");
  GLint view_loc = glGetUniformLocation(shader_program, "uView");
  GLint projection_loc = glGetUniformLocation(shader_program, "uProjection");

  printf("\n=== 3D TRANSFORMATION PIPELINE DEMONSTRATION ===\n");
  printf("Controls:\n");
  printf("- ESC: Exit\n");
  printf("- The triangle rotates to show 3D perspective\n");
  printf("- Colors fade with distance from camera\n\n");

  printf("Matrix Setup:\n");
  printf("- Model: Rotation around Y-axis\n");
  printf("- View: Camera at (3, 2, 3) looking at origin\n");
  printf("- Projection: 45° FOV, 4:3 aspect ratio\n\n");

  // Main render loop
  while (!glfwWindowShouldClose(window)) {
    // Handle input
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, 1);

    // Clear screen
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use shader program
    glUseProgram(shader_program);

    // Create transformation matrices
    Mat4 model, view, projection;

    // 1. Model matrix - rotate triangle around Y-axis
    // float time = (float)glfwGetTime();
    float time = 0.2;
    // (float)glfwGetTime();
    mat4_identity(&model);
    // Simple Y-axis rotation
    float angle = time * 0.5f; // Slow rotation
    model.m[0] = cosf(angle);
    model.m[8] = sinf(angle);
    model.m[2] = -sinf(angle);
    model.m[10] = cosf(angle);

    // 2. View matrix - camera positioned to look at the triangle
    Vec3 camera_pos = {3.0f, 2.0f, 8.0f};
    Vec3 target = {0.0f, 0.0f, 0.0f};
    Vec3 up = {0.0f, 1.0f, 0.0f};
    mat4_lookat(&view, camera_pos, target, up);

    // 3. Projection matrix - perspective projection
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    float aspect = (float)width / (float)height;
    mat4_perspective(&projection, 45.0f * M_PI / 180.0f, aspect, 0.1f, 100.0f);

    // Send matrices to shader
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, model.m);
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(projection_loc, 1, GL_FALSE, projection.m);

    // Draw triangle
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteProgram(shader_program);

  glfwTerminate();
  return 0;
}

// Compilation instructions:
/*
To compile and run this example:

1. Install dependencies (Ubuntu/Debian):
   sudo apt-get install libglfw3-dev libglew-dev

2. Compile:
   gcc -o 3d_demo main.c -lglfw -lGLEW -lGL -lm

3. Run:
   ./3d_demo

For other systems:
- Windows: Use libraries like freeglut + GLEW, or SDL2 + GLEW
- macOS: Install via Homebrew: brew install glfw glew

Key Features Demonstrated:
1. Vertex shader receives 3D positions and transforms them through
Model-View-Projection pipeline
2. Each vertex passes through: Object -> World -> Camera -> Clip -> Screen
coordinates
3. Fragment shader adds depth-based coloring to visualize 3D effect
4. Triangle rotates to show perspective projection working
5. Colors are interpolated across the triangle surface

Mathematical Pipeline in Action:
- Model Matrix: Rotates triangle in world space
- View Matrix: Positions camera to look at triangle from (3,2,3)
- Projection Matrix: Applies perspective with 45° field of view
- GPU automatically handles perspective divide and viewport transform

Watch how the triangle changes shape as it rotates - this demonstrates
the perspective projection working correctly!
*/

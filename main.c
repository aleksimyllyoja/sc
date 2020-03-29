#include "glad.h"
#include <stdio.h>

#include <float.h>
#include <limits.h>
#include <math.h>

#include <string.h>
#include <stdlib.h>

#ifdef _WIN64
#define atoll(S) _atoi64(S)
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include <GLFW/glfw3.h>

#include "trackball.h"

typedef struct {
  float x;
  float y;
  float z;
} Vec3;

typedef struct {
  GLuint vertex_buffer;
  GLuint element_buffer;
  int vertex_count;
  int element_count;
} Shape;

typedef struct {
  Vec3 point;
  Vec3 slope;
  GLuint vertex_buffer;
} Line;

typedef struct {
  Vec3 v;
  int defined;
} MaybeVec3;

const float PI = 3.1415926f;
const float SQ2 = 1.41421356237f;

static Shape shape;
static Line line;

static int width = 768;
static int height = 768;

static float prevMouseX, prevMouseY;
static int mouseLeftPressed;
static int mouseMiddlePressed;
static int mouseRightPressed;
static float curr_quat[4];
static float prev_quat[4];
static float eye[3], lookat[3], up[3];
static GLFWwindow* gWindow;

static void check_gl_errors(const char* desc) {
  GLenum e = glGetError();
  if (e != GL_NO_ERROR) {
    printf("OpenGL error in \"%s\": %d (%d)\n", desc, e, e);
    exit(20);
  }
}

static Vec3 vec3(float x, float y, float z) {
  Vec3 v = {x, y, z};
  return v;
}

static void printvec3(Vec3 v) {
  printf("%f, %f, %f\n", v.x, v.y, v.z);
}

static Vec3 subtract(Vec3 v1, Vec3 v2) {
  Vec3 v3;
  v3.x = v1.x-v2.x;
  v3.y = v1.y-v2.y;
  v3.z = v1.z-v2.z;

  return v3;
}

static Vec3 add(Vec3 v1, Vec3 v2) {
  Vec3 v3;
  v3.x = v1.x+v2.x;
  v3.y = v1.y+v2.y;
  v3.z = v1.z+v2.z;

  return v3;
}

static float dot(Vec3 v1, Vec3 v2) {
  return v1.x*v2.x+v1.y*v2.y+v1.z*v2.z;
}

static Vec3 cross(Vec3 v1, Vec3 v2) {
  Vec3 v3;
  v3.x = v1.y*v2.z-v1.z*v2.y;
  v3.y = v1.z*v2.x-v1.x*v2.z;
  v3.z = v1.x*v2.y-v1.y*v2.x;

  return v3;
}

static Vec3 multiply(Vec3 v1, float s) {
  Vec3 v2;
  v2.x = v1.x*s;
  v2.y = v1.y*s;
  v2.z = v1.z*s;
  return v2;
}

static Vec3 divide(Vec3 v1, float s) {
  Vec3 v2;
  v2.x = v1.x/s;
  v2.y = v1.y/s;
  v2.z = v1.z/s;
  return v2;
}

static float length(Vec3 v) {
  return sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
}

static float _area(Vec3 x1, Vec3 x2, Vec3 a, Vec3 w) {
  return 0.5*(dot(cross(subtract(x1, a), subtract(x2, a)), w));
}

static Vec3 normalize(Vec3 v) {
  Vec3 v2;
  float l = length(v);
  v2.x = v.x / l;
  v2.y = v.y / l;
  v2.z = v.z / l;

  return v2;
}

static float distance(Vec3 v1, Vec3 v2) {
  return length(subtract(v1, v2));
}

static void update_buffers(int ci, int cv, GLushort ebi, GLushort vbi, float* vertices, GLushort* indices) {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebi);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(ci * sizeof(GLushort)), indices, GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbi);
  glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(cv * sizeof(float)), vertices, GL_STATIC_DRAW);
}

static void update_shape_buffers(float* vertices, GLushort* indices) {
  update_buffers(shape.element_count, shape.vertex_count, shape.element_buffer, shape.vertex_buffer, vertices, indices);
}

static void gen_tetrahedron(float s) {
  float vertices[12] = {
    1*s, 1*s, 1*s,
    -1*s, -1*s, 1*s,
    1*s, -1*s, -1*s,
    -1*s, 1*s, -1*s
  };

  GLushort indices[12] = {
    0, 1, 2,
    0, 2, 3,
    0, 1, 3,
    2, 3, 1
  };

  shape.element_buffer = 0;
  shape.vertex_buffer = 0;
  shape.element_count = 12;
  shape.vertex_count = 12;

  glGenBuffers(1, &shape.element_buffer);
  glGenBuffers(1, &shape.vertex_buffer);

  update_shape_buffers(vertices, indices);
}

static void reshape_callback(GLFWwindow* window, int w, int h) {
  int fb_w, fb_h;
  glfwGetFramebufferSize(window, &fb_w, &fb_h);

  glViewport(0, 0, fb_w, fb_h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0, (GLdouble)w / (GLdouble)h, (GLdouble)0.01f,
                 (GLdouble)100.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  width = w;
  height = h;
}

static void keyboard_callback(GLFWwindow* window, int key, int scancode, int action,
                         int mods) {
  (void)window;
  (void)scancode;
  (void)mods;
  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE)
      glfwSetWindowShouldClose(window, GL_TRUE);
  }
}

static void click_callback(GLFWwindow* window, int button, int action, int mods) {
  (void)window;
  (void)mods;
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      mouseLeftPressed = 1;
      trackball(prev_quat, 0.0, 0.0, 0.0, 0.0);
    } else if (action == GLFW_RELEASE) {
      mouseLeftPressed = 0;
    }
  }
  if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (action == GLFW_PRESS) {
      mouseRightPressed = 1;
    } else if (action == GLFW_RELEASE) {
      mouseRightPressed = 0;
    }
  }
  if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
    if (action == GLFW_PRESS) {
      mouseMiddlePressed = 1;
    } else if (action == GLFW_RELEASE) {
      mouseMiddlePressed = 0;
    }
  }
}

static void motion_callback(GLFWwindow* window, double mouse_x, double mouse_y) {
  float rotScale = 1.2f;
  float transScale = 2.1f;

  (void)window;

  if (mouseLeftPressed) {
    trackball(prev_quat, rotScale * (2.0f * prevMouseX - width) / (float)width,
              rotScale * (height - 2.0f * prevMouseY) / (float)height,
              rotScale * (2.0f * (float)mouse_x - width) / (float)width,
              rotScale * (height - 2.0f * (float)mouse_y) / (float)height);

    add_quats(prev_quat, curr_quat, curr_quat);
  } else if (mouseMiddlePressed) {
    eye[0] -= transScale * ((float)mouse_x - prevMouseX) / (float)width;
    lookat[0] -= transScale * ((float)mouse_x - prevMouseX) / (float)width;
    eye[1] += transScale * ((float)mouse_y - prevMouseY) / (float)height;
    lookat[1] += transScale * ((float)mouse_y - prevMouseY) / (float)height;
  } else if (mouseRightPressed) {
    eye[2] += transScale * ((float)mouse_y - prevMouseY) / (float)height;
    lookat[2] += transScale * ((float)mouse_y - prevMouseY) / (float)height;
  }

  prevMouseX = (float)mouse_x;
  prevMouseY = (float)mouse_y;
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
  eye[2] -= yoffset/40.0;
}

static void set_line_buffer(Line* l) {
  Vec3 v1 = add(l->point, multiply(l->slope, 10));
  Vec3 v2 = add(l->point, multiply(l->slope, -10));
  float vertices[6] = {v1.x, v1.y, v1.z, v2.x, v2.y, v2.z};

  l->vertex_buffer=0;
  glGenBuffers(1, &l->vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, l->vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(6 * sizeof(float)), &vertices, GL_STATIC_DRAW);
}

static void draw_shape(const Shape* s) {
  /*glPolygonMode(GL_FRONT, GL_FILL);
  glPolygonMode(GL_BACK, GL_FILL);

  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(0.0, 0.0);
  glColor3f(0.8f, 0.8f, 0.8f);

  glPointSize(10);

  glBindBuffer(GL_ARRAY_BUFFER, s->vertex_buffer);
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, (const void*)0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->element_buffer);
  glDrawElements(GL_TRIANGLES, s->element_count, GL_UNSIGNED_SHORT, 0);


  printf("COUNTS: %d, %d\n", s->vertex_count, s->element_count);

  check_gl_errors("drawarrays");
  */
  /* wireframe */

  glDisable(GL_POLYGON_OFFSET_FILL);
  glPolygonMode(GL_FRONT, GL_LINE);
  glPolygonMode(GL_BACK, GL_LINE);

  glColor3f(0.4f, 0.4f, 0.4f);

  glBindBuffer(GL_ARRAY_BUFFER, s->vertex_buffer);
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, (const void*)0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->element_buffer);
  glDrawElements(GL_TRIANGLES, s->element_count, GL_UNSIGNED_SHORT, 0);

  check_gl_errors("drawarrays");
}

static void draw_ray(const Line* l) {
  glColor3f(1.0f, 0.4f, 0.4f);

  glLineWidth(3);
  glBindBuffer(GL_ARRAY_BUFFER, l->vertex_buffer);
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, (const void*)0);
  glDrawArrays(GL_LINES, 0, 2);

  check_gl_errors("drawarrays");
}

static void init() {
  trackball(curr_quat, 0, 0, 0, 0);

  eye[0] = 0.0f;
  eye[1] = 0.0f;
  eye[2] = 4.0f;

  lookat[0] = 0.0f;
  lookat[1] = 0.0f;
  lookat[2] = 0.0f;

  up[0] = 0.0f;
  up[1] = 1.0f;
  up[2] = 0.0f;
}

static void set_callbacks() {
  glfwSetWindowSizeCallback(gWindow, reshape_callback);
  glfwSetKeyCallback(gWindow, keyboard_callback);
  glfwSetMouseButtonCallback(gWindow, click_callback);
  glfwSetCursorPosCallback(gWindow, motion_callback);
  glfwSetScrollCallback(gWindow, scroll_callback);
}

static MaybeVec3 tlis(Vec3 x1, Vec3 x2, Vec3 x3, Vec3 a, Vec3 b) {
  float A1 = _area(x2, x3, a, b);
  float A2 = _area(x3, x1, a, b);
  float A3 = _area(x1, x2, a, b);

  float A = A1+A2+A3;
  Vec3 num = add(add(multiply(x1, A1), multiply(x2, A2)), multiply(x3, A3));

  MaybeVec3 ret;
  ret.defined = 0;

  if(A1 <= 0 && A2 <= 0 && A3 <= 0) {
    ret.v = divide(num, A);
    ret.defined = 1;
  } else if(A1 >= 0 && A2 >= 0 && A3 >= 0) {
    ret.v = divide(num, A);
    ret.defined = 1;
  }
  return ret;
}

static Vec3 triangle_center(Vec3 a, Vec3 b, Vec3 c) {
  return divide(add(add(a, b), c), 3);
}

static int get_closest_vertex(Vec3 p, int vertex_count, float* vertices) {
  float d = INFINITY;
  float dn;
  int i, besti;

  for(i=0; i<vertex_count; i=i+3) {
    dn = distance(p, vec3(vertices[i], vertices[i+1], vertices[i+2]));
    if(dn < d) {
      d = dn;
      besti = i;
    }
  }
  return besti;
}

static void search_intersections() {
  float vertices[shape.vertex_count];
  GLushort indices[shape.element_count];

  Vec3 v1, v2, v3, cv;
  MaybeVec3 spoint;
  int i, closest, face_index;

  glBindBuffer(GL_ARRAY_BUFFER, shape.vertex_buffer);
  glGetBufferSubData(GL_ARRAY_BUFFER, 0, shape.vertex_count*sizeof(float), &vertices);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape.element_buffer);
  glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, shape.element_count*sizeof(GLushort), &indices);

  for(i=0; i<shape.element_count; i=i+3) {
    v1 = vec3(vertices[3*indices[i]], vertices[3*indices[i]+1], vertices[3*indices[i]+2]);
    v2 = vec3(vertices[3*indices[i+1]], vertices[3*indices[i+1]+1], vertices[3*indices[i+1]+2]);
    v3 = vec3(vertices[3*indices[i+2]], vertices[3*indices[i+2]+1], vertices[3*indices[i+2]+2]);

    spoint = tlis(v1, v2, v3, line.point, line.slope);

    if(spoint.defined) {
      float* new_vertices = malloc((shape.vertex_count)*sizeof(float)+3);
      GLushort* new_indices = malloc((shape.element_count)*sizeof(float)+6);
      GLushort ni = shape.element_count-3;
      Vec3 tc;
      int j;

      closest = get_closest_vertex(spoint.v, shape.vertex_count, vertices);

      cv = vec3(vertices[closest], vertices[closest+1], vertices[closest+2]);
      cv = add(cv, line.slope);

      /*vertices[closest] = cv.x;
      vertices[closest+1] = cv.y;
      vertices[closest+2] = cv.y;
      */

      tc = triangle_center(v1, v2, v3);
      printf("Adding triangle center: "); printvec3(tc);

      memcpy(new_vertices, vertices, shape.vertex_count*sizeof(float));
      new_vertices[shape.vertex_count] = tc.x;
      new_vertices[shape.vertex_count+1] = tc.y;
      new_vertices[shape.vertex_count+2] = tc.z;

      memcpy(new_indices, indices, (i-1)*sizeof(float));
      memcpy(&new_indices[i], &indices[i+3], (shape.element_count-i-3)*sizeof(GLushort));

      new_indices[ni] = indices[i];
      new_indices[ni+1] = indices[i+1];
      new_indices[ni+2] = shape.vertex_count/3;

      new_indices[ni+3] = indices[i+1];
      new_indices[ni+4] = indices[i+2];
      new_indices[ni+5] = shape.vertex_count/3;

      new_indices[ni+6] = indices[i+2];
      new_indices[ni+7] = indices[i];
      new_indices[ni+8] = shape.vertex_count/3;

      shape.vertex_count+=3;
      shape.element_count+=6;

      printf("UPDATING BUFFERS\n");
      update_shape_buffers(new_vertices, new_indices);

      printf("FREEING MEMORY\n");

      free(new_vertices);
      free(new_indices);
      break;
    }
  }

}

int main(int argc, char** argv) {
  Vec3 a = vec3(0.0, 0, 0.30);
  Vec3 b = vec3(-0.1, 0.8, 1);

  line.point = a;
  line.slope = normalize(b);

  init();

  printf("Initialize GLFW...\n");

  if (!glfwInit()) {
    printf("Failed to initialize GLFW.\n");
    return -1;
  }

  gWindow = glfwCreateWindow(width, height, "Sculpt", NULL, NULL);
  if (gWindow == NULL) {
    printf("Failed to open GLFW window.\n");
    glfwTerminate();
    return 1;
  }

  set_callbacks();

  glfwSwapInterval(1);
  glfwMakeContextCurrent(gWindow);

  if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
    printf("Failed to initialize GLAD.\n");
    return -1;
  }

  reshape_callback(gWindow, width, height);

  {
    float bmin[3]={-1, -1, -1}, bmax[3]={1, 1, 1};
    float maxExtent;

    maxExtent = 0.5f * (bmax[0] - bmin[0]);
    if(maxExtent < 0.5f * (bmax[1] - bmin[1])) {
      maxExtent = 0.5f * (bmax[1] - bmin[1]);
    }
    if(maxExtent < 0.5f * (bmax[2] - bmin[2])) {
      maxExtent = 0.5f * (bmax[2] - bmin[2]);
    }

    gen_tetrahedron(0.5);
    set_line_buffer(&line);

    search_intersections();
    search_intersections();

    while(glfwWindowShouldClose(gWindow) == GL_FALSE) {
      GLfloat mat[4][4];
      glfwPollEvents();
      glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glEnable(GL_DEPTH_TEST);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      gluLookAt((GLdouble)eye[0], (GLdouble)eye[1], (GLdouble)eye[2],
                (GLdouble)lookat[0], (GLdouble)lookat[1], (GLdouble)lookat[2],
                (GLdouble)up[0], (GLdouble)up[1], (GLdouble)up[2]);
      build_rotmatrix(mat, curr_quat);
      glMultMatrixf(&mat[0][0]);

      glScalef(1.0f / maxExtent, 1.0f / maxExtent, 1.0f / maxExtent);

      glTranslatef(-0.5f * (bmax[0] + bmin[0]), -0.5f * (bmax[1] + bmin[1]),
                   -0.5f * (bmax[2] + bmin[2]));
      draw_shape(&shape);
      draw_ray(&line);

      glfwSwapBuffers(gWindow);
    }
  }

  glfwTerminate();
  return 0;
}

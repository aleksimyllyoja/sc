#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  float x;
  float y;
  float z;
} Vec3;

typedef struct {
  float* vertices;
  int* indices;
  int vertex_count;
  int index_count;
} Shape;

typedef struct {
  Vec3 point;
  Vec3 slope;
  int direction;
} Line;

typedef struct {
  Vec3 a;
  Vec3 b;
  Vec3 c;
} Triangle;

typedef struct {
  Vec3 v;
  int defined;
} MaybeVec3;

Vec3 vec3(float x, float y, float z) {
  Vec3 v = {x, y, z};
  return v;
}

Vec3 subtract(Vec3 v1, Vec3 v2) {
  Vec3 v3 = {v1.x-v2.x, v1.y-v2.y, v1.z-v2.z};
  return v3;
}

Vec3 add(Vec3 v1, Vec3 v2) {
  Vec3 v3 = {v1.x+v2.x, v1.y+v2.y, v1.z+v2.z};
  return v3;
}

float dot(Vec3 v1, Vec3 v2) {
  return v1.x*v2.x+v1.y*v2.y+v1.z*v2.z;
}

Vec3 cross(Vec3 v1, Vec3 v2) {
  Vec3 v3 = {
    v1.y*v2.z-v1.z*v2.y,
    v1.z*v2.x-v1.x*v2.z,
    v1.x*v2.y-v1.y*v2.x
  };
  return v3;
}

Vec3 multiply(Vec3 v1, float s) {
  Vec3 v2 = {v1.x*s, v1.y*s, v1.z*s};
  return v2;
}

Vec3 divide(Vec3 v1, float s) {
  Vec3 v2 = {v1.x/s, v1.y/s, v1.z/s};
  return v2;
}

float length(Vec3 v) {
  return sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
}

Vec3 triangle_center(Vec3 a, Vec3 b, Vec3 c) {
  return divide(add(add(a, b), c), 3);
}

float _area(Vec3 x1, Vec3 x2, Vec3 a, Vec3 w) {
  return 0.5*(dot(cross(subtract(x1, a), subtract(x2, a)), w));
}

Vec3 normalize(Vec3 v) {
  float l = length(v);
  Vec3 v2 = {v.x/l, v.y/l, v.z/l};
  return v2;
}

float distance(Vec3 v1, Vec3 v2) {
  return length(subtract(v1, v2));
}

Vec3 midpoint(Vec3 v1, Vec3 v2) {
  return divide(add(v1, v2), 2);
}

float triangle_area(Vec3 v1, Vec3 v2, Vec3 v3) {
  Vec3 v12 = subtract(v2, v1);
  Vec3 v13 = subtract(v3, v1);
  return 0.5*length(cross(v12, v13));
}

MaybeVec3 triangle_line_intersection(Vec3 x1, Vec3 x2, Vec3 x3, Vec3 a, Vec3 b) {
  float A1 = _area(x2, x3, a, b);
  float A2 = _area(x3, x1, a, b);
  float A3 = _area(x1, x2, a, b);

  float A = A1+A2+A3;
  Vec3 num = add(add(multiply(x1, A1), multiply(x2, A2)), multiply(x3, A3));

  MaybeVec3 ret;
  ret.v = vec3(0, 0, 0);
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

MaybeVec3 find_intersection(Shape* s, Line line) {
  Vec3 v1, v2, v3, cp;
  MaybeVec3 ipoint, ret;
  float d = INFINITY, dn;
  int i;
  float* vs = s->vertices;
  int* is = s->indices;

  ret.defined = 0;

  cp = add(line.point, multiply(line.slope, line.direction*10));

  for(i=0; i<s->index_count; i=i+3) {
    v1 = vec3(vs[3*is[i]], vs[3*is[i]+1], vs[3*is[i]+2]);
    v2 = vec3(vs[3*is[i+1]], vs[3*is[i+1]+1], vs[3*is[i+1]+2]);
    v3 = vec3(vs[3*is[i+2]], vs[3*is[i+2]+1], vs[3*is[i+2]+2]);

    ipoint = triangle_line_intersection(v1, v2, v3, line.point, line.slope);
    if(ipoint.defined) {
      dn = distance(cp, vec3(ipoint.v.x, ipoint.v.y, ipoint.v.z));
      if(dn < d) {
        ret = ipoint;
        d = dn;
      }
    }
  }
  return ret;
}

Shape* create_cube2() {
  float vertices[24] = {
    1.000000, -1.000000, -1.000000,
    1.000000, -1.000000, 1.000000,
    -1.000000, -1.000000, 1.000000,
    -1.000000, -1.000000, -1.000000,
    1.000000, 1.000000, -1.000000,
    1.000000, 1.000000, 1.000000,
    -1.000000, 1.000000, 1.000000,
    -1.000000, 1.000000, -1.000000
  };

  int indices[36] = {
    1, 2, 3,
    7, 6, 5,
    4, 5, 1,
    5, 6, 2,
    2, 6, 7,
    0, 3, 7,
    0, 1, 3,
    4, 7, 5,
    0, 4, 1,
    1, 5, 2,
    3, 2, 7,
    4, 0, 7,
  };


  Shape* cube = malloc(sizeof(Shape));
  cube->vertices = malloc(sizeof(float)*24);
  cube->indices = malloc(sizeof(int)*36);

  memcpy(cube->vertices, &vertices, sizeof(float)*24);
  memcpy(cube->indices, &indices, sizeof(int)*36);

  cube->vertex_count = 24;
  cube->index_count = 36;

  return cube;
}

Shape* create_cube() {
  float vertices[24578*3] = {
    #include "data/vertices2"
  };

  int indices[49152*3] = {
    #include "data/indices2"
  };

  Shape* cube = malloc(sizeof(Shape));
  cube->vertices = malloc(sizeof(float)*24578*3);
  cube->indices = malloc(sizeof(int)*49152*3);

  memcpy(cube->vertices, &vertices, sizeof(float)*24578*3);
  memcpy(cube->indices, &indices, sizeof(int)*49152*3);

  cube->vertex_count = 24578*3;
  cube->index_count = 49152*3;

  return cube;
}

void free_shape(Shape* s) {
  free(s->vertices);
  free(s->indices);
  free(s);
}

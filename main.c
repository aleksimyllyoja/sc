#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "sculpt.h"

#define PI 3.1415
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MINMAX(a, b, x) MAX(a, MIN(b, x))

void write_shape(Shape* s) {
  int i;
  FILE *f = fopen("shape-out.obj", "w");

  for(i=0; i<s->vertex_count; i=i+3) {
    fprintf(f, "v %f %f %f\n", s->vertices[i], s->vertices[i+1], s->vertices[i+2]);
  }
  for(i=0; i<s->index_count; i=i+3) {
    fprintf(f, "f %i %i %i\n", s->indices[i]+1, s->indices[i+1]+1, s->indices[i+2]+1);
  }

  fclose(f);
}

static int find_vertex(float* vertices, int vc, Vec3 v) {
  int vi = -1;
  for(int i=0; i<vc; i+=3) {
    if(vertices[i] == v.x &&
       vertices[i+1] == v.y &&
       vertices[i+2] == v.z
    ) vi=i;
  }
  return vi;
}

static void printvec3(Vec3 v) {
  printf("%f, %f, %f\n", v.x, v.y, v.z);
}

static void set_vertex(float* vertices, int i, Vec3 v) {
  vertices[i] = v.x;
  vertices[i+1] = v.y;
  vertices[i+2] = v.z;
}

static int set_or_get(float* vertices, int* vc, Vec3 p) {
  int itmp = find_vertex(vertices, *vc, p);
  if(itmp == -1) {
    set_vertex(vertices, *vc, p);
    *vc += 3;
    return ((*vc)-3)/3;
  } else return itmp/3;
}

static void subdivide(Shape *s) {
  int vc = s->vertex_count;
  int ic = s->index_count;
  float* vs = s->vertices;
  int* is = s->indices;

  float* new_vertices = malloc(vc*4*sizeof(float));
  int* new_indices = malloc(ic*4*sizeof(int));

  int i, new_vi = s->vertex_count;
  int inc = 0;

  memcpy(new_vertices, vs, vc*sizeof(float));

  for(i=0; i<ic; i=i+3) {
    Vec3 v1, v2, v3, p1, p2, p3;
    int i1 = is[i], i2 = is[i+1], i3 = is[i+2];
    int p1i, p2i, p3i;

    v1 = vec3(vs[3*i1], vs[3*i1+1], vs[3*i1+2]);
    v2 = vec3(vs[3*i2], vs[3*i2+1], vs[3*i2+2]);
    v3 = vec3(vs[3*i3], vs[3*i3+1], vs[3*i3+2]);

    p1 = midpoint(v1, v2);
    p2 = midpoint(v2, v3);
    p3 = midpoint(v3, v1);

    p1i = set_or_get(new_vertices, &new_vi, p1);
    p2i = set_or_get(new_vertices, &new_vi, p2);
    p3i = set_or_get(new_vertices, &new_vi, p3);

    new_indices[i*4] = i1;
    new_indices[i*4+1] = p1i;
    new_indices[i*4+2] = p3i;

    new_indices[i*4+3] = p1i;
    new_indices[i*4+4] = i2;
    new_indices[i*4+5] = p2i;

    new_indices[i*4+6] = p3i;
    new_indices[i*4+7] = p2i;
    new_indices[i*4+8] = i3;

    new_indices[i*4+9] = p1i;
    new_indices[i*4+10] = p2i;
    new_indices[i*4+11] = p3i;
    inc+=12;
  }

  free(s->vertices);
  free(s->indices);

  s->vertex_count = new_vi;
  s->index_count = inc;

  s->vertices = new_vertices;
  s->indices = new_indices;
}

static float scalef(float x, float brush_size, float amount) {
  return (sin(PI/2+MINMAX(0, PI/2, x/brush_size)))*amount;
}

static void sculpt(Shape* s, Line line, Vec3 cp, float brush_size, float amount) {
  int i;
  Vec3 nl = normalize(line.slope);
  for(i=0; i<s->vertex_count; i=i+3) {
    Vec3 v = vec3(s->vertices[i], s->vertices[i+1], s->vertices[i+2]);
    float d = distance(v, cp);
    Vec3 av = multiply(nl, scalef(d, brush_size, amount));

    Vec3 nv = add(v, av);

    s->vertices[i] = nv.x;
    s->vertices[i+1] = nv.y;
    s->vertices[i+2] = nv.z;
  }
}

static void sph(Shape* s) {
  int i;
  float r, phi, theta;
  float r2 = 2;
  for(i=0; i<s->vertex_count; i=i+3) {
    Vec3 v = vec3(s->vertices[i], s->vertices[i+1], s->vertices[i+2]);

    r = length(v);
    v = multiply(v, 1+(r2-r)*0.5);

    s->vertices[i] = v.x;
    s->vertices[i+1] = v.y;
    s->vertices[i+2] = v.z;
  }
}

void calc_areas(Shape *s, float* areas) {

}

int main(int argc, char** argv) {
  Shape* cube = create_cube();
  MaybeVec3 ins;
  Line line;

  subdivide(cube);
  subdivide(cube);

  subdivide(cube);
  subdivide(cube);
  subdivide(cube);
  sph(cube);

  /*
  line.point = vec3(-0.4, 0.3, 0);
  line.slope = vec3(0, 0, 1);
  line.direction = 1;
  ins = find_intersection(cube, line);
  if(ins.defined) sculpt(cube, line, ins.v, 0.04, -0.5);

  line.point = vec3(0.4, 0.3, 0);
  ins = find_intersection(cube, line);
  if(ins.defined) sculpt(cube, line, ins.v, 0.04, -0.5);

  line.point = vec3(0, -0.3, 0);
  ins = find_intersection(cube, line);
  if(ins.defined) sculpt(cube, line, ins.v, 0.2, -0.5);

  line.point = vec3(0, 0, 0);
  ins = find_intersection(cube, line);
  if(ins.defined) sculpt(cube, line, ins.v, 0.2, 0.5);
  */
  write_shape(cube);
  free_shape(cube);
}

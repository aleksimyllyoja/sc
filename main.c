#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "sculpt.h"

#define PI 3.1415
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MINMAX(a, b, x) MAX(a, MIN(b, x))

int verticesa_alloc = 0;
int indices_alloc = 0;

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

void write_shape2(Shape* s) {
  int i;
  FILE *f = fopen("data/vertices2", "w");

  for(i=0; i<s->vertex_count; i=i+3) {
    fprintf(f, "%f, %f, %f,\n", s->vertices[i], s->vertices[i+1], s->vertices[i+2]);
  }
  /*
  for(i=0; i<s->index_count; i=i+3) {
    fprintf(f, "f %i %i %i\n", s->indices[i]+1, s->indices[i+1]+1, s->indices[i+2]+1);
  }*/
  fclose(f);

  FILE *f2 = fopen("data/indices2", "w");

  for(i=0; i<s->index_count; i=i+3) {
    fprintf(f, "%i, %i, %i,\n", s->indices[i], s->indices[i+1], s->indices[i+2]);
  }

  fclose(f2);
}

static int find_vertex(float* vertices, int vc, Vec3 v) {
  int vi = -1, i;
  for(i=0; i<vc; i+=3) {
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
  float r;
  float r2 = 3;
  for(i=0; i<s->vertex_count; i=i+3) {
    Vec3 v = vec3(s->vertices[i], s->vertices[i+1], s->vertices[i+2]);

    r = length(v);
    v = multiply(v, r2/r);

    s->vertices[i] = v.x;
    s->vertices[i+1] = v.y;
    s->vertices[i+2] = v.z;
  }
}

static void sculpt_on_intersection(Shape *s, Line l, float brush_radius, float amount) {
  MaybeVec3 ins = find_intersection(s, l);
  if(ins.defined) sculpt(s, l, ins.v, brush_radius, amount);
}

int main(int argc, char** argv) {
  Shape* cube = create_cube();
  Line line;

  if(argc<1) {
    printf("No file given\n");
    return -1;
  } else {
    printf("reading %s\n", argv[1]);
  }

  FILE *file;
  char buf[100];
  file = fopen(argv[1], "r");

  line.direction = 1;

  while(fgets(buf, sizeof buf, file) != NULL) {
    char command[2];
    strncpy(command, buf, 2);

    if(strncmp(command, "sd", 2) == 0) {
      //printf("subdividing\n");
      subdivide(cube);
    }

    if(strncmp(command, "ip", 2) == 0) {
      float x=0, y=0, z=0;
      sscanf(&buf[2], "%f %f %f", &x, &y, &z);
      //printf("setting instrument position: %f %f %f\n", x, y, z);
      line.point = vec3(x, y, z);
    }

    if(strncmp(command, "is", 2) == 0) {
      float x=0, y=0, z=0;
      sscanf(&buf[2], "%f %f %f", &x, &y, &z);
      //printf("setting instrument slope: %f %f %f\n", x, y, z);
      line.slope = vec3(x, y, z);
    }

    if(strncmp(command, "id", 2) == 0) {
      int d=1;
      sscanf(&buf[2], "%d", &d);
      //printf("setting instrument direction: %d\n", d);
      line.direction = d;
    }

    if(strncmp(command, "sc", 2) == 0) {
      float radius=0, amount=0;
      sscanf(&buf[2], "%f %f", &radius, &amount);
      //printf("sculpting with radius=%f, amount=%f\n", radius, amount);
      sculpt_on_intersection(cube, line, radius, amount);
    }
    if(strncmp(command, "si", 2) == 0) {
      MaybeVec3 ins = find_intersection(cube, line);
      if(ins.defined) line.point = ins.v;
    }

    if(strncmp(command, "sm", 2) == 0) {
      //printf("smoothing\n");
      sph(cube);
    }
  }

  fclose(file);

  printf("vertex count: %d\n", cube->vertex_count);
  printf("index count: %d\n", cube->index_count);

  write_shape(cube);
  //write_shape2(cube);
  free_shape(cube);
}

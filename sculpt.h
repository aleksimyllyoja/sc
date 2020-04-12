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

Vec3 vec3(float x, float y, float z);
Vec3 subtract(Vec3 v1, Vec3 v2);
Vec3 add(Vec3 v1, Vec3 v2);
float dot(Vec3 v1, Vec3 v2);
Vec3 cross(Vec3 v1, Vec3 v2);
Vec3 multiply(Vec3 v1, float s);
Vec3 divide(Vec3 v1, float s);
float length(Vec3 v);
Vec3 normalize(Vec3 v);
Vec3 midpoint(Vec3 v1, Vec3 v2);
float distance(Vec3 v1, Vec3 v2);

Vec3 triangle_center(Vec3 a, Vec3 b, Vec3 c);
float triangle_area(Vec3 v1, Vec3 v2, Vec3 v3);

MaybeVec3 triangle_line_intersection(Vec3 x1, Vec3 x2, Vec3 x3, Vec3 a, Vec3 b);
MaybeVec3 find_intersection(Shape* s, Line line);

Shape* create_cube();
void free_shape(Shape *s);

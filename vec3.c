typedef struct {
  float x;
  float y;
  float z;
} Vec3;

static float length(Vec3 v) {
  return sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
}

static void normalize(Vec3* v) {

}

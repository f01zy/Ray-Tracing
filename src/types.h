#ifndef TYPES_H
#define TYPES_H

#include <cglm/cglm.h>
#include <stdbool.h>

typedef enum {
  OBJECT_SPHERE,
} ObjectType;

typedef struct {
  vec3 position;
  vec3 normal;
  float t;
  bool is_front;
} Hit;

typedef struct {
  vec3 origin;
  vec3 direction;
} Ray;

typedef struct {
  vec3 position;
  float radius;
} Sphere;

typedef struct {
  ObjectType type;
  union {
    Sphere sphere;
  } as;
} Object;

typedef struct {
  vec3 position;
  vec3 pixel_delta_u, pixel_delta_v;
  vec3 origin_pixel_center;
  float aspect_ratio;
  float focal_length;
  int samples_per_pixel;
  int max_ray_depth;
  int width;
  int height;
} Camera;

#endif

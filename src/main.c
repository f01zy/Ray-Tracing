#include <cglm/cglm.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define WIDTH        (400)
#define ASPECT_RATIO (16.0f / 9.0f)

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

Object objects[] = {
  {
    .type = OBJECT_SPHERE,
    .as.sphere = {
      .position = {0.0f, 0.0f, -1.0f},
      .radius = 0.5f,
    },
  },
};

void ray_at(Ray *ray, float t, vec3 dest) {
  vec3 temp;
  glm_vec3_scale(ray->direction, t, temp);
  glm_vec3_add(ray->origin, temp, dest);
}

void hit_set_normal(Ray *ray, Hit *hit, vec3 normal) {
  hit->is_front = glm_vec3_dot(ray->direction, normal) < 0.0f;
  if (!hit->is_front) glm_vec3_scale(normal, -1.0f, normal);
  glm_vec3_copy(normal, hit->normal);
}

bool hit_sphere(Ray *ray, Sphere *sphere, float t_min, float t_max, Hit *hit) {
  vec3 oc;
  glm_vec3_sub(sphere->position, ray->origin, oc);
  float a = glm_vec3_dot(ray->direction, ray->direction);
  float h = glm_vec3_dot(ray->direction, oc);
  float c = glm_vec3_dot(oc, oc) - sphere->radius * sphere->radius;
  float d = h * h - a * c;
  if (d < 0.0f) return false;
  float sqrtd = sqrt(d);
  float t1 = (h - d) / a, t2 = (h + d) / a, t = -1.0f;
  if (t2 > t_min && t2 < t_max) t = t2;
  if (t1 > t_min && t1 < t_max) t = t1;
  if (t == -1.0f) return false;
  ray_at(ray, t, hit->position);
  vec3 normal;
  glm_vec3_sub(hit->position, sphere->position, normal);
  glm_vec3_normalize(normal);
  hit_set_normal(ray, hit, normal);
  hit->t = t;
  return true;
}

bool hit_world(Ray *ray, float t_min, float t_max, Hit *rec) {
  size_t objects_count = sizeof(objects) / sizeof(*objects);
  float far = t_max;
  bool is_hit_anything = false;
  for (int i = 0; i < objects_count; i++) {
    bool is_hit = false;
    switch (objects[i].type) {
    case OBJECT_SPHERE:
      is_hit = hit_sphere(ray, &objects[i].as.sphere, t_min, far, rec);
      break;
    }
    if (is_hit) {
      is_hit_anything = true;
      far = rec->t;
    }
  }
  return is_hit_anything;
}

void ray_color(Ray *ray, vec3 dest) {
  Hit rec;
  if (hit_world(ray, 0.0f, INFINITY, &rec)) {
    vec3 color;
    glm_vec3_adds(rec.normal, 1.0f, color);
    glm_vec3_scale(color, 0.5f, dest);
    return;
  }
  vec3 blue = {0.1f, 0.6f, 0.9f};
  vec3 white = {1.0f, 1.0f, 1.0f};
  vec3 norm;
  glm_vec3_normalize_to(ray->direction, norm);
  float a = 0.5f * (norm[1] + 1.0f);
  glm_vec3_scale(blue, a, blue);
  glm_vec3_scale(white, 1.0f - a, white);
  glm_vec3_add(blue, white, dest);
}

void write_color(FILE *stream, vec3 color) {
  uint8_t rbyte = color[0] * 255;
  uint8_t gbyte = color[1] * 255;
  uint8_t bbyte = color[2] * 255;
  char buf[64];
  size_t len = snprintf(buf, sizeof(buf), "%d %d %d\n", rbyte, gbyte, bbyte);
  fwrite(buf, 1, len, stream);
}

int main(int argc, char **argv) {
  int width = WIDTH;
  int height = width / ASPECT_RATIO;

  vec3 temp;
  float viewport_height = 2.0f;
  float viewport_width = viewport_height * ((float)width / height);
  float focal_length = 1.0f;
  vec3 camera_position = {0.0f, 0.0f, 0.0f};

  vec3 viewport_u = {viewport_width, 0.0f, 0.0f};
  vec3 viewport_v = {0.0f, -viewport_height, 0.0f};
  vec3 pixel_delta_u, pixel_delta_v;
  glm_vec3_scale(viewport_u, 1.0f / width, pixel_delta_u);
  glm_vec3_scale(viewport_v, 1.0f / height, pixel_delta_v);

  vec3 focal_vec = {0.0f, 0.0f, focal_length};
  vec3 viewport_up_left;
  glm_vec3_sub(camera_position, focal_vec, viewport_up_left);
  glm_vec3_add(viewport_u, viewport_v, temp);
  glm_vec3_scale(temp, 0.5f, temp);
  glm_vec3_sub(viewport_up_left, temp, viewport_up_left);

  vec3 origin_pixel_center;
  glm_vec3_add(pixel_delta_u, pixel_delta_v, temp);
  glm_vec3_scale(temp, 0.5f, temp);
  glm_vec3_add(viewport_up_left, temp, origin_pixel_center);

  printf("P3\n%d %d\n255\n", width, height);
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      vec3 pixel_center;
      vec3 ray_direction;
      glm_vec3_scale(pixel_delta_u, j, temp);
      glm_vec3_add(origin_pixel_center, temp, pixel_center);
      glm_vec3_scale(pixel_delta_v, i, temp);
      glm_vec3_add(pixel_center, temp, pixel_center);
      glm_vec3_sub(pixel_center, camera_position, ray_direction);

      Ray ray;
      glm_vec3_copy(camera_position, ray.origin);
      glm_vec3_copy(ray_direction, ray.direction);

      vec3 color;
      ray_color(&ray, color);
      write_color(stdout, color);
    }
  }
}

#include <cglm/cglm.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define WIDTH             (400)
#define ASPECT_RATIO      (16.0f / 9.0f)
#define FOCAL_LENGTH      (1.0f)
#define SAMPLES_PER_PIXEL (100)

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
  int width;
  int height;
} Camera;

Object objects[] = {
  {
    .type = OBJECT_SPHERE,
    .as.sphere = {
      .position = {0.0f, 0.0f, -3.0f},
      .radius = 1.0f,
    },
  },
  {
    .type = OBJECT_SPHERE,
    .as.sphere = {
      .position = {0.0f, -51.0f, -3.0f},
      .radius = 50.0f,
    },
  },
};

float random_number() { return rand() / (RAND_MAX + 1.0f); }

float random_range(float min, float max) { return min + (max - min) * random_number(); }

void write_color(FILE *stream, vec3 color) {
  uint8_t rbyte = 255 * glm_clamp(color[0], 0.0f, 0.999f);
  uint8_t gbyte = 255 * glm_clamp(color[1], 0.0f, 0.999f);
  uint8_t bbyte = 255 * glm_clamp(color[2], 0.0f, 0.999f);
  char buf[64];
  size_t len = snprintf(buf, sizeof(buf), "%d %d %d\n", rbyte, gbyte, bbyte);
  fwrite(buf, 1, len, stream);
}

bool interval_contains(vec2 interval, float x) { return x >= interval[0] && x <= interval[1]; }

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

bool hit_sphere(Ray *ray, Sphere *sphere, vec2 interval, Hit *hit) {
  vec3 co;
  glm_vec3_sub(sphere->position, ray->origin, co);
  float a = glm_vec3_dot(ray->direction, ray->direction);
  float h = glm_vec3_dot(ray->direction, co);
  float c = glm_vec3_dot(co, co) - sphere->radius * sphere->radius;
  float d = h * h - a * c;
  if (d < 0.0f) return false;
  float sqrtd = sqrt(d);
  float t1 = (h - sqrtd) / a, t2 = (h + sqrtd) / a, t = -1.0f;
  if (interval_contains(interval, t2)) t = t2;
  if (interval_contains(interval, t1)) t = t1;
  if (t == -1.0f) return false;
  ray_at(ray, t, hit->position);
  vec3 normal;
  glm_vec3_sub(hit->position, sphere->position, normal);
  glm_vec3_normalize(normal);
  hit_set_normal(ray, hit, normal);
  hit->t = t;
  return true;
}

bool hit_world(Ray *ray, vec2 interval, Hit *rec) {
  size_t objects_count = sizeof(objects) / sizeof(*objects);
  bool is_hit_anything = false;
  for (int i = 0; i < objects_count; i++) {
    bool is_hit = false;
    switch (objects[i].type) {
    case OBJECT_SPHERE:
      is_hit = hit_sphere(ray, &objects[i].as.sphere, interval, rec);
      break;
    }
    if (is_hit) {
      is_hit_anything = true;
      interval[1] = rec->t;
    }
  }
  return is_hit_anything;
}

void camera_ray_color(Ray *ray, vec3 dest) {
  Hit rec;
  vec2 interval = {0.0f, INFINITY};
  if (hit_world(ray, interval, &rec)) {
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

void camera_get_ray(Camera *camera, int i, int j, Ray *dest) {
  vec2 offset = {
    (float)j + random_number() - 0.5f,
    (float)i + random_number() - 0.5f,
  };
  vec3 pixel_center, ray_direction, temp;
  glm_vec3_scale(camera->pixel_delta_u, offset[0], temp);
  glm_vec3_add(camera->origin_pixel_center, temp, pixel_center);
  glm_vec3_scale(camera->pixel_delta_v, offset[1], temp);
  glm_vec3_add(pixel_center, temp, pixel_center);
  glm_vec3_sub(pixel_center, camera->position, ray_direction);
  glm_vec3_copy(camera->position, dest->origin);
  glm_vec3_normalize_to(ray_direction, dest->direction);
}

void camera_initialize(Camera *camera, int width, float aspect_ratio, float focal_length, float samples_per_pixel) {
  camera->aspect_ratio = aspect_ratio;
  camera->focal_length = focal_length;
  camera->samples_per_pixel = samples_per_pixel;
  camera->width = width;
  camera->height = camera->width / camera->aspect_ratio;
  float viewport_height = 2.0f;
  float viewport_width = viewport_height * ((float)camera->width / camera->height);

  vec3 temp;
  vec3 position = {0.0f, 0.0f, 0.0f};
  glm_vec3_copy(position, camera->position);

  vec3 viewport_u = {viewport_width, 0.0f, 0.0f};
  vec3 viewport_v = {0.0f, -viewport_height, 0.0f};
  glm_vec3_scale(viewport_u, 1.0f / camera->width, camera->pixel_delta_u);
  glm_vec3_scale(viewport_v, 1.0f / camera->height, camera->pixel_delta_v);

  vec3 focal_vec = {0.0f, 0.0f, camera->focal_length};
  vec3 viewport_up_left;
  glm_vec3_sub(camera->position, focal_vec, viewport_up_left);
  glm_vec3_add(viewport_u, viewport_v, temp);
  glm_vec3_scale(temp, 0.5f, temp);
  glm_vec3_sub(viewport_up_left, temp, viewport_up_left);
  glm_vec3_add(camera->pixel_delta_u, camera->pixel_delta_v, temp);
  glm_vec3_scale(temp, 0.5f, temp);
  glm_vec3_add(viewport_up_left, temp, camera->origin_pixel_center);
}

void camera_render(Camera *camera) {
  printf("P3\n%d %d\n255\n", camera->width, camera->height);
  Ray ray;
  vec3 sample;
  for (int i = 0; i < camera->height; i++) {
    for (int j = 0; j < camera->width; j++) {
      vec3 color = GLM_VEC3_ZERO_INIT;
      for (int k = 0; k < camera->samples_per_pixel; k++) {
        camera_get_ray(camera, i, j, &ray);
        camera_ray_color(&ray, sample);
        glm_vec3_add(color, sample, color);
      }
      glm_vec3_scale(color, 1.0f / camera->samples_per_pixel, color);
      write_color(stdout, color);
    }
  }
}

int main(int argc, char **argv) {
  srand(time(NULL));
  Camera camera;
  camera_initialize(&camera, WIDTH, ASPECT_RATIO, FOCAL_LENGTH, SAMPLES_PER_PIXEL);
  camera_render(&camera);
}

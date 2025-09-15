// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "include/cente_influence.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static void gaussian_kernel_1d(float sigma, float **out_kernel,
                               int *out_radius) {
  int radius = (int)ceilf(3.0f * sigma);
  if (radius < 1)
    radius = 1;
  int size = 2 * radius + 1;
  float *k = (float *)malloc((size_t)size * sizeof(float));
  if (!k) {
    *out_kernel = NULL;
    *out_radius = 0;
    return;
  }
  float sum = 0.0f;
  float inv2s2 = 1.0f / (2.0f * sigma * sigma);
  for (int i = -radius, j = 0; i <= radius; i++, j++) {
    float v = expf(-(float)(i * i) * inv2s2);
    k[j] = v;
    sum += v;
  }
  for (int j = 0; j < size; j++)
    k[j] /= sum;
  *out_kernel = k;
  *out_radius = radius;
}

InfluenceMap *influence_create(int width, int height) {
  InfluenceMap *m = (InfluenceMap *)calloc(1, sizeof(InfluenceMap));
  if (!m)
    return NULL;
  m->width = width;
  m->height = height;
  size_t n = (size_t)width * (size_t)height;
  m->iyou = (float *)calloc(n, sizeof(float));
  m->irival = (float *)calloc(n, sizeof(float));
  m->t = (float *)calloc(n, sizeof(float));
  m->grad = (float *)calloc(n, sizeof(float));
  if (!m->iyou || !m->irival || !m->t || !m->grad) {
    influence_destroy(m);
    return NULL;
  }
  return m;
}

void influence_destroy(InfluenceMap *m) {
  if (!m)
    return;
  free(m->iyou);
  free(m->irival);
  free(m->t);
  free(m->grad);
  free(m);
}

static void convolve_separable(const float *src, float *tmp, float *dst, int w,
                               int h, const float *k, int r) {
  // Horizontal
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      float s = 0.0f;
      for (int i = -r; i <= r; i++) {
        int xx = x + i;
        if (xx < 0)
          xx = 0;
        if (xx >= w)
          xx = w - 1;
        s += src[y * w + xx] * k[i + r];
      }
      tmp[y * w + x] = s;
    }
  }
  // Vertical
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      float s = 0.0f;
      for (int i = -r; i <= r; i++) {
        int yy = y + i;
        if (yy < 0)
          yy = 0;
        if (yy >= h)
          yy = h - 1;
        s += tmp[yy * w + x] * k[i + r];
      }
      dst[y * w + x] = s;
    }
  }
}

static void normalize_01(float *a, int n) {
  float mn = 1e30f, mx = -1e30f;
  for (int i = 0; i < n; i++) {
    if (a[i] < mn)
      mn = a[i];
    if (a[i] > mx)
      mx = a[i];
  }
  float d = mx - mn;
  if (d < 1e-6f)
    d = 1.0f;
  for (int i = 0; i < n; i++)
    a[i] = (a[i] - mn) / d;
}

void compute_influence_full(const Board *b, int self_id, float sigma,
                            InfluenceMap *out) {
  int w = b->width, h = b->height;
  int n = w * h;
  float *src_you = (float *)calloc((size_t)n, sizeof(float));
  float *src_rival = (float *)calloc((size_t)n, sizeof(float));
  float *tmp = (float *)malloc((size_t)n * sizeof(float));
  if (!src_you || !src_rival || !tmp) {
    free(src_you);
    free(src_rival);
    free(tmp);
    return;
  }
  memset(out->iyou, 0, (size_t)n * sizeof(float));
  memset(out->irival, 0, (size_t)n * sizeof(float));
  memset(out->t, 0, (size_t)n * sizeof(float));
  memset(out->grad, 0, (size_t)n * sizeof(float));

  // Seeds: heads contribute strong influence; bodies weak; free points mild
  // based on value
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int c = b->cells[y * w + x];
      float v = (c > 0) ? (0.2f + 0.8f * (float)c / 255.0f) : 0.05f;
      src_you[y * w + x] = 0.0f;
      src_rival[y * w + x] = 0.0f;
      (void)v;
    }
  }
  // Heads as strong Gaussians
  for (int p = 0; p < b->num_players; p++) {
    int hx = b->head_x[p];
    int hy = b->head_y[p];
    int idxh = hy * w + hx;
    if (p == self_id)
      src_you[idxh] = 1.0f;
    else
      src_rival[idxh] = 1.0f;
  }

  float *k;
  int r;
  gaussian_kernel_1d(sigma, &k, &r);
  if (!k) {
    free(src_you);
    free(src_rival);
    free(tmp);
    return;
  }
  convolve_separable(src_you, tmp, out->iyou, w, h, k, r);
  convolve_separable(src_rival, tmp, out->irival, w, h, k, r);

  // Territory and gradient magnitude
  for (int i = 0; i < n; i++) {
    float t = out->iyou[i] - out->irival[i];
    out->t[i] = t > 0.0f ? t : 0.0f;
  }
  normalize_01(out->iyou, n);
  normalize_01(out->irival, n);
  normalize_01(out->t, n);

  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int i = y * w + x;
      float tx1 = out->t[i] - out->t[y * w + (x > 0 ? x - 1 : x)];
      float ty1 = out->t[i] - out->t[(y > 0 ? y - 1 : y) * w + x];
      float gx = fabsf(tx1);
      float gy = fabsf(ty1);
      out->grad[i] = sqrtf(gx * gx + gy * gy);
    }
  }
  normalize_01(out->grad, n);

  free(src_you);
  free(src_rival);
  free(tmp);
  free(k);
}

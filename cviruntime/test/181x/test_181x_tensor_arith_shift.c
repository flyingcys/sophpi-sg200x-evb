#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

static void tl_arith_shift_ref(
    uint8_t *ref_high, uint8_t *ref_low,
    uint8_t *a_high, uint8_t *a_low,
    uint8_t *bits, uint64_t size)
{
  for (uint64_t i = 0; i < size; i++) {
    int32_t ta = ((int8_t)a_high[i] << 8) + a_low[i];
    int32_t tbits = (int8_t)bits[i];

    /*
     * Yes, a @tbits bigger than zero means shifting LEFT,
     * no matter whether the shift type is arithmetic
     * RIGHT shift or logic RIGHT shift.
     */
    int32_t res;
    if (tbits >= 0)
      res = ta << tbits;
    else
      res = ta >> -tbits;

    ref_high[i] = (res >> 8) & 0xff;
    ref_low[i] = res & 0xff;
  }
}

static int test_tl_arith_shift(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, int eu_align)
{
  int ret = 0;
  int n = 2; // 3 -> 2 for 1810
  int c = 39;
  int h = 7;
  int w = 37;

  cvk_tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  uint32_t size = n * c * h * w;
  uint8_t *a_high_data = (uint8_t *)malloc(size);
  uint8_t *a_low_data = (uint8_t *)malloc(size);
  uint8_t *bits_data = (uint8_t *)malloc(size);
  uint8_t *ref_high_data = (uint8_t *)malloc(size);
  uint8_t *ref_low_data = (uint8_t *)malloc(size);
  if (!a_high_data || !a_low_data || !bits_data || !ref_high_data || !ref_low_data) {
    ret = -1;
    goto fail_exit;
  }

  for (uint32_t i = 0; i < size; i++) {
    a_high_data[i] = 240 + i;
    a_low_data[i] = 200 + i;
    bits_data[i] = (i % 33) - 16;
  }

  tl_arith_shift_ref(
      ref_high_data, ref_low_data,
      a_high_data, a_low_data,
      bits_data, size);

  cvk_tl_t *tl_a_low = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_a_high = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_bits = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_res_low = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_res_high = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  if (!tl_a_low || !tl_a_high || !tl_bits || !tl_res_low || !tl_res_high) {
    ret = -1;
    goto fail_exit_2;
  }

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_a_low, a_low_data);
  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_a_high, a_high_data);
  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_bits, bits_data);
  cvk_tiu_arith_shift_param_t p8;
  p8.res_high = tl_res_high;
  p8.res_low = tl_res_low;
  p8.a_high = tl_a_high;
  p8.a_low = tl_a_low;
  p8.bits = tl_bits;
  cvk_ctx->ops->tiu_arith_shift(cvk_ctx, &p8);
  uint8_t *res_high_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_res_high);
  uint8_t *res_low_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_res_low);

  for (uint32_t i = 0; i < size; i++) {
    if (res_high_data[i] != ref_high_data[i]) {
      fprintf(stderr, "comparing failed at res_high_data[%u], got %d, exp %d\n",
             i, res_high_data[i], ref_high_data[i]);
      ret = -1;
      break;
    }
    if (res_low_data[i] != ref_low_data[i]) {
      fprintf(stderr, "comparing failed at res_low_data[%u], got %d, exp %d\n",
             i, res_low_data[i], ref_low_data[i]);
      ret = -1;
      break;
    }
  }

  free(res_high_data);
  free(res_low_data);

fail_exit_2:
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_res_high);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_res_low);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_bits);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_a_high);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_a_low);

fail_exit:
  free(a_high_data);
  free(a_low_data);
  free(bits_data);
  free(ref_high_data);
  free(ref_low_data);

  return ret;
}

int main(int argc, char **argv)
{
  int ret = 0;
  CVI_RT_HANDLE rt_handle = NULL;
  cvk_context_t *cvk_ctx = NULL;
 
  if (!argc)
    return -1;
  if (!argv)
    return -1;

  CVI_RT_Init(&rt_handle);
  cvk_ctx = CVI_RT_RegisterKernel(rt_handle, CMDBUF_SIZE);
  if (!rt_handle || !cvk_ctx) {
    printf("%s fail\n", __FILENAME__);
    return -1;
  }

  ret |= test_tl_arith_shift(rt_handle, cvk_ctx, 0);
  ret |= test_tl_arith_shift(rt_handle, cvk_ctx, 1);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
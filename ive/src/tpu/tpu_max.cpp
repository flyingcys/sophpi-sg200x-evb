#include "tpu/tpu_cmp.hpp"

#include <string.h>

void IveTPUMax::setKernelSize(uint32_t kz) {
  m_kernel_info.size = kz;
  m_kernel_info.pad[0] = 0;
  m_kernel_info.pad[1] = 0;
  m_kernel_info.pad[2] = 0;
  m_kernel_info.pad[3] = 0;
}

int IveTPUMax::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_cmdbuf_subfix = "max";
  m_slice_info.io_fmt = CVK_FMT_U8;
  m_slice_info.nums_of_tl = 2;
  m_kernel_info.nums_of_kernel = 0;
  return CVI_SUCCESS;
}

int IveTPUMax::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                        const std::vector<cvk_tg_shape_t> &tg_in_slices,
                        const std::vector<cvk_tg_shape_t> &tg_out_slices,
                        std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                        const bool enable_cext) {
  cvk_tl_shape_t tl_shape, tl_shape_out;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  tl_shape_out.n = tg_out_slices[0].n;
  tl_shape_out.c = tg_out_slices[0].c;
  tl_shape_out.h = tg_out_slices[0].h;
  tl_shape_out.w = tg_out_slices[0].w;
  auto *tl_input = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1);
  auto *tl_output = allocTLMem(cvk_ctx, tl_shape_out, CVK_FMT_U8, 1);

  m_p_max.ifmap = tl_input;
  m_p_max.ofmap = tl_output;
  m_p_max.kh = m_kernel_info.size;
  m_p_max.kw = m_kernel_info.size;
  m_p_max.pad_top = 0;
  m_p_max.pad_bottom = 0;
  m_p_max.pad_left = 0;
  m_p_max.pad_right = 0;
  m_p_max.stride_w = 1;
  m_p_max.stride_h = 1;
  m_p_max.ins_fp = 0;  // Useless in max pooling but we still give it a value.

  tl_in_idx->push_back(0);
  tl_out_idx->push_back(1);
  return CVI_SUCCESS;
}

void IveTPUMax::operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, uint32_t ping_idx) {
  cvk_ctx->ops->tiu_max_pooling(cvk_ctx, &m_p_max);
}
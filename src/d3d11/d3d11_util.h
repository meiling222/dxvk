#pragma once

#include "../dxvk/dxvk_device.h"

#include "../dxbc/dxbc_util.h"

#include "d3d11_include.h"

namespace dxvk {
  
  template<typename T>
  UINT CompactSparseList(T* pData, UINT Mask) {
    uint32_t count = 0;
    
    for (uint32_t id : bit::BitMask(Mask))
      pData[count++] = pData[id];

    return count;
  }

  HRESULT DecodeSampleCount(
          UINT                      Count,
          VkSampleCountFlagBits*    pCount);
    
  VkSamplerAddressMode DecodeAddressMode(
          D3D11_TEXTURE_ADDRESS_MODE  mode);
  
  VkCompareOp DecodeCompareOp(
          D3D11_COMPARISON_FUNC     Mode);
  
  VkConservativeRasterizationModeEXT DecodeConservativeRasterizationMode(
          D3D11_CONSERVATIVE_RASTERIZATION_MODE Mode);

  VkFormatFeatureFlags2 GetBufferFormatFeatures(
          UINT                      BindFlags);

  VkFormatFeatureFlags2 GetImageFormatFeatures(
          UINT                      BindFlags);
  
  VkFormat GetPackedDepthStencilFormat(
          DXGI_FORMAT               Format);

  /**
   * \brief Translates D3D11 shader stage to corresponding Vulkan stage
   *
   * \param [in] ProgramType DXBC program type
   * \returns Corresponding Vulkan shader stage
   */
  constexpr VkShaderStageFlagBits GetShaderStage(DxbcProgramType ProgramType) {
    switch (ProgramType) {
      case DxbcProgramType::VertexShader:   return VK_SHADER_STAGE_VERTEX_BIT;
      case DxbcProgramType::HullShader:     return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
      case DxbcProgramType::DomainShader:   return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
      case DxbcProgramType::GeometryShader: return VK_SHADER_STAGE_GEOMETRY_BIT;
      case DxbcProgramType::PixelShader:    return VK_SHADER_STAGE_FRAGMENT_BIT;
      case DxbcProgramType::ComputeShader:  return VK_SHADER_STAGE_COMPUTE_BIT;
      default:                              return VkShaderStageFlagBits(0);
    }
  }

}
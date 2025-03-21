#include "dxvk_barrier.h"
#include "dxvk_buffer.h"
#include "dxvk_device.h"

#include <algorithm>

namespace dxvk {
  
  DxvkBuffer::DxvkBuffer(
          DxvkDevice*           device,
    const DxvkBufferCreateInfo& createInfo,
          DxvkMemoryAllocator&  memAlloc,
          VkMemoryPropertyFlags memFlags)
  : m_device        (device),
    m_info          (createInfo),
    m_memAlloc      (&memAlloc),
    m_memFlags      (memFlags),
    m_shaderStages  (util::shaderStages(createInfo.stages)) {
    // Align slices so that we don't violate any alignment
    // requirements imposed by the Vulkan device/driver
    VkDeviceSize sliceAlignment = computeSliceAlignment();
    m_physSliceLength = createInfo.size;
    m_physSliceStride = align(createInfo.size, sliceAlignment);
    m_physSliceCount  = std::max<VkDeviceSize>(1, 256 / m_physSliceStride);

    // Limit size of multi-slice buffers to reduce fragmentation
    constexpr VkDeviceSize MaxBufferSize = 256 << 10;

    m_physSliceMaxCount = MaxBufferSize >= m_physSliceStride
      ? MaxBufferSize / m_physSliceStride
      : 1;

    // Allocate the initial set of buffer slices. Only clear
    // buffer memory if there is more than one slice, since
    // we expect the client api to initialize the first slice.
    m_buffer = allocBuffer(m_physSliceCount, m_physSliceCount > 1);

    DxvkBufferSliceHandle slice;
    slice.handle = m_buffer.buffer;
    slice.offset = 0;
    slice.length = m_physSliceLength;
    slice.mapPtr = m_buffer.memory.mapPtr(0);

    m_physSlice = slice;
    m_lazyAlloc = m_physSliceCount > 1;
  }


  DxvkBuffer::~DxvkBuffer() {
    auto vkd = m_device->vkd();

    for (const auto& buffer : m_buffers)
      vkd->vkDestroyBuffer(vkd->device(), buffer.buffer, nullptr);
    vkd->vkDestroyBuffer(vkd->device(), m_buffer.buffer, nullptr);
  }
  
  
  DxvkBufferHandle DxvkBuffer::allocBuffer(VkDeviceSize sliceCount, bool clear) const {
    auto vkd = m_device->vkd();

    VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    info.size                  = m_physSliceStride * sliceCount;
    info.usage                 = m_info.usage;
    info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    
    DxvkBufferHandle handle;

    if (vkd->vkCreateBuffer(vkd->device(),
          &info, nullptr, &handle.buffer) != VK_SUCCESS) {
      throw DxvkError(str::format(
        "DxvkBuffer: Failed to create buffer:"
        "\n  size:  ", info.size,
        "\n  usage: ", info.usage));
    }
    
    VkMemoryDedicatedRequirements dedicatedRequirements = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS };
    VkMemoryRequirements2 memReq = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2, &dedicatedRequirements };
    
    VkBufferMemoryRequirementsInfo2 memReqInfo = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2 };
    memReqInfo.buffer = handle.buffer;
    
    VkMemoryDedicatedAllocateInfo dedMemoryAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO };
    dedMemoryAllocInfo.buffer = handle.buffer;

    vkd->vkGetBufferMemoryRequirements2(
       vkd->device(), &memReqInfo, &memReq);

    // Use high memory priority for GPU-writable resources
    bool isGpuWritable = (m_info.access & (
      VK_ACCESS_SHADER_WRITE_BIT |
      VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT)) != 0;

    DxvkMemoryFlags hints(DxvkMemoryFlag::GpuReadable);

    if (isGpuWritable)
      hints.set(DxvkMemoryFlag::GpuWritable);

    // Staging buffers that can't even be used as a transfer destinations
    // are likely short-lived, so we should put them on a separate memory
    // pool in order to avoid fragmentation
    if ((DxvkBarrierSet::getAccessTypes(m_info.access) == DxvkAccess::Read)
     && (m_info.usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT))
      hints.set(DxvkMemoryFlag::Transient);

    // Ask driver whether we should be using a dedicated allocation
    handle.memory = m_memAlloc->alloc(&memReq.memoryRequirements,
      dedicatedRequirements, dedMemoryAllocInfo, m_memFlags, hints);
    
    if (vkd->vkBindBufferMemory(vkd->device(), handle.buffer,
        handle.memory.memory(), handle.memory.offset()) != VK_SUCCESS)
      throw DxvkError("DxvkBuffer: Failed to bind device memory");
    
    if (clear && (m_memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
      std::memset(handle.memory.mapPtr(0), 0, info.size);

    return handle;
  }


  VkDeviceSize DxvkBuffer::computeSliceAlignment() const {
    const auto& devInfo = m_device->properties();

    VkDeviceSize result = sizeof(uint32_t);

    if (m_info.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
      result = std::max(result, devInfo.core.properties.limits.minUniformBufferOffsetAlignment);
      result = std::max(result, devInfo.extRobustness2.robustUniformBufferAccessSizeAlignment);
    }

    if (m_info.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
      result = std::max(result, devInfo.core.properties.limits.minStorageBufferOffsetAlignment);
      result = std::max(result, devInfo.extRobustness2.robustStorageBufferAccessSizeAlignment);
    }

    if (m_info.usage & (VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)) {
      result = std::max(result, devInfo.core.properties.limits.minTexelBufferOffsetAlignment);
      result = std::max(result, VkDeviceSize(16));
    }

    if (m_info.usage & (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
     && m_info.size > (devInfo.core.properties.limits.optimalBufferCopyOffsetAlignment / 2))
      result = std::max(result, devInfo.core.properties.limits.optimalBufferCopyOffsetAlignment);

    // For some reason, Warhammer Chaosbane breaks otherwise
    if (m_info.usage & (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
      result = std::max(result, VkDeviceSize(256));

    if (m_memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
      result = std::max(result, devInfo.core.properties.limits.nonCoherentAtomSize);
      result = std::max(result, VkDeviceSize(64));
    }

    return result;
  }



  
  DxvkBufferView::DxvkBufferView(
    const Rc<vk::DeviceFn>&         vkd,
    const Rc<DxvkBuffer>&           buffer,
    const DxvkBufferViewCreateInfo& info)
  : m_vkd(vkd), m_info(info), m_buffer(buffer),
    m_bufferSlice (getSliceHandle()),
    m_bufferView  (createBufferView(m_bufferSlice)) {
    
  }
  
  
  DxvkBufferView::~DxvkBufferView() {
    if (m_views.empty()) {
      m_vkd->vkDestroyBufferView(
        m_vkd->device(), m_bufferView, nullptr);
    } else {
      for (const auto& pair : m_views) {
        m_vkd->vkDestroyBufferView(
          m_vkd->device(), pair.second, nullptr);
      }
    }
  }
  
  
  VkBufferView DxvkBufferView::createBufferView(
    const DxvkBufferSliceHandle& slice) {
    VkBufferViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO };
    viewInfo.buffer = slice.handle;
    viewInfo.format = m_info.format;
    viewInfo.offset = slice.offset;
    viewInfo.range  = slice.length;
    
    VkBufferView result = VK_NULL_HANDLE;

    if (m_vkd->vkCreateBufferView(m_vkd->device(),
          &viewInfo, nullptr, &result) != VK_SUCCESS) {
      throw DxvkError(str::format(
        "DxvkBufferView: Failed to create buffer view:",
        "\n  Offset: ", viewInfo.offset,
        "\n  Range:  ", viewInfo.range,
        "\n  Format: ", viewInfo.format));
    }

    return result;
  }


  void DxvkBufferView::updateBufferView(
    const DxvkBufferSliceHandle& slice) {
    if (m_views.empty())
      m_views.insert({ m_bufferSlice, m_bufferView });
    
    m_bufferSlice = slice;
    
    auto entry = m_views.find(slice);
    if (entry != m_views.end()) {
      m_bufferView = entry->second;
    } else {
      m_bufferView = createBufferView(m_bufferSlice);
      m_views.insert({ m_bufferSlice, m_bufferView });
    }
  }
  
  
  DxvkBufferTracker:: DxvkBufferTracker() { }
  DxvkBufferTracker::~DxvkBufferTracker() { }
  
  
  void DxvkBufferTracker::reset() {
    std::sort(m_entries.begin(), m_entries.end(),
      [] (const Entry& a, const Entry& b) {
        return a.slice.handle < b.slice.handle;
      });

    for (const auto& e : m_entries)
      e.buffer->freeSlice(e.slice);
      
    m_entries.clear();
  }
  
}
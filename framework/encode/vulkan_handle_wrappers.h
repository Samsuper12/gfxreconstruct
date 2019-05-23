/*
** Copyright (c) 2019 LunarG, Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef GFXRECON_ENCODE_VULKAN_HANDLE_WRAPPERS_H
#define GFXRECON_ENCODE_VULKAN_HANDLE_WRAPPERS_H

#include "encode/vulkan_state_info.h"
#include "format/format.h"
#include "util/defines.h"
#include "util/memory_output_stream.h"

#include "vulkan/vulkan.h"

#include <limits>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(encode)

//
// Handle wrappers for storing object state information with object handles.
//

template <typename T>
struct HandleWrapper
{
    typedef T HandleType;

    // Dispatch table for dispatch handles.
    void* dispatch_table_{ nullptr };

    // Standard state info required for all handles.
    HandleType        handle{ VK_NULL_HANDLE }; // Original handle value provided by the driver.
    format::HandleId  handle_id{ 0 };           // Globally unique ID assigned to the handle by the layer.
    format::ApiCallId create_call_id{ format::ApiCallId::ApiCall_Unknown };
    CreateParameters  create_parameters;
};

//
// Type definitions for handle wrappers that do not require additional state info.
//

// clang-format off
struct QueueWrapper                     : public HandleWrapper<VkQueue> {};
struct BufferViewWrapper                : public HandleWrapper<VkBufferView> {};
struct ShaderModuleWrapper              : public HandleWrapper<VkShaderModule> {};
struct PipelineCacheWrapper             : public HandleWrapper<VkPipelineCache> {};
struct SamplerWrapper                   : public HandleWrapper<VkSampler> {};
struct SamplerYcbcrConversionWrapper    : public HandleWrapper<VkSamplerYcbcrConversion> {};
struct DescriptorUpdateTemplateWrapper  : public HandleWrapper<VkDescriptorUpdateTemplate> {};
struct DebugReportCallbackEXTWrapper    : public HandleWrapper<VkDebugReportCallbackEXT> {};
struct DebugUtilsMessengerEXTWrapper    : public HandleWrapper<VkDebugUtilsMessengerEXT> {};
struct ValidationCacheEXTWrapper        : public HandleWrapper<VkValidationCacheEXT> {};
struct IndirectCommandsLayoutNVXWrapper : public HandleWrapper<VkIndirectCommandsLayoutNVX> {};

// This handle type is retrieved and has no destroy function. The handle wrapper will be owned by its VkPhysicalDevice
// handle wrapper, which will filter duplicate handle retrievals and ensure that the wrapper is destroyed.
struct DisplayKHRWrapper : public HandleWrapper<VkDisplayKHR> {};

// This handle type has a create function, but no destroy function. The handle wrapper will be owned by its parent VkPhysicalDevice
// handle wrapper, which will ensure it is destroyed.
struct DisplayModeKHRWrapper            : public HandleWrapper<VkDisplayModeKHR> {};
// clang-format on

// Handle alias types for extension handle types that have been promoted to core types.
typedef VkSamplerYcbcrConversionKHR   SamplerYcbcrConversionKHRWrapper;
typedef VkDescriptorUpdateTemplateKHR DescriptorUpdateTemplateKHRWrapper;

//
// Declarations for handle wrappers that require additional state info.
//

// This handle type is retrieved and has no destroy function. The handle wrapper will be owned by the VkInstance
// handle wrapper, which will ensure it is destroyed when the VkInstance handle wrapper is destroyed.
struct PhysicalDeviceWrapper : public HandleWrapper<VkPhysicalDevice>
{
    std::vector<DisplayKHRWrapper*>     child_displays;
    std::vector<DisplayModeKHRWrapper*> child_display_modes;

    // Track memory types for use when creating snapshots of buffer and image resource memory content.
    std::vector<VkMemoryType> memory_types;

    // Track queue family properties retrieval call data to write to state snapshot after physical device creation.
    // The queue family data is only written to the state snapshot if the application made the API call to retrieve it.
    format::ApiCallId                           queue_family_properties_call_id{ format::ApiCallId::ApiCall_Unknown };
    uint32_t                                    queue_family_properties_count{ 0 };
    std::unique_ptr<VkQueueFamilyProperties[]>  queue_family_properties;
    std::unique_ptr<VkQueueFamilyProperties2[]> queue_family_properties2;
    std::vector<std::unique_ptr<VkQueueFamilyCheckpointPropertiesNV>> queue_family_checkpoint_properties;
};

struct InstanceWrapper : public HandleWrapper<VkInstance>
{
    std::vector<PhysicalDeviceWrapper*> child_physical_devices;
};

struct DeviceWrapper : public HandleWrapper<VkDevice>
{
    PhysicalDeviceWrapper*                     physical_device{ nullptr };
    std::vector<QueueWrapper*>                 child_queues;
    std::unordered_map<VkQueue, QueueWrapper*> queues;
};

struct FenceWrapper : public HandleWrapper<VkFence>
{
    // Signaled state at creation to be compared with signaled state at snapshot write. If states are different, the
    // create parameters will need to be modified to reflect the state at snapshot write.
    bool     created_signaled{ false };
    VkDevice device{ VK_NULL_HANDLE };
};

struct EventWrapper : public HandleWrapper<VkEvent>
{
    VkDevice device{ VK_NULL_HANDLE };
};

struct BufferWrapper : public HandleWrapper<VkBuffer>
{
    VkDevice       bind_device{ VK_NULL_HANDLE };
    VkDeviceMemory bind_memory{ VK_NULL_HANDLE };
    VkDeviceSize   bind_offset{ 0 };
    uint32_t       queue_family_index{ 0 };
    VkDeviceSize   created_size{ 0 };
};

struct ImageWrapper : public HandleWrapper<VkImage>
{
    VkDevice              bind_device{ VK_NULL_HANDLE };
    VkDeviceMemory        bind_memory{ VK_NULL_HANDLE };
    VkDeviceSize          bind_offset{ 0 };
    uint32_t              queue_family_index{ 0 };
    VkImageType           image_type{ VK_IMAGE_TYPE_2D };
    VkFormat              format{ VK_FORMAT_UNDEFINED };
    VkExtent3D            extent{ 0, 0, 0 };
    uint32_t              mip_levels{ 0 };
    uint32_t              array_layers{ 0 };
    VkSampleCountFlagBits samples{};
    VkImageTiling         tiling{};
    VkImageLayout         current_layout{ VK_IMAGE_LAYOUT_UNDEFINED };
};

struct ImageViewWrapper : public HandleWrapper<VkImageView>
{
    // Store handle to associated image for tracking render pass layout transitions.
    VkImage image{ VK_NULL_HANDLE };
};

struct FramebufferWrapper : public HandleWrapper<VkFramebuffer>
{
    // TODO: This only requires the unique sequence number once handles are fully wrapped.
    VkRenderPass      render_pass{ VK_NULL_HANDLE };
    format::HandleId  render_pass_id{ 0 };
    format::ApiCallId render_pass_create_call_id{ format::ApiCallId::ApiCall_Unknown };
    CreateParameters  render_pass_create_parameters;

    // Track handles of image attachments for processing render pass layout transitions.
    std::vector<VkImage> attachments;
};

struct SemaphoreWrapper : public HandleWrapper<VkSemaphore>
{
    // Track semaphore signaled state. State is signaled when a sempahore is submitted to QueueSubmit, QueueBindSparse,
    // AcquireNextImageKHR, or AcquireNextImage2KHR as a signal semaphore. State is not signaled when a semaphore is
    // submitted to QueueSubmit, QueueBindSparse, or QueuePresentKHR as a wait semaphore. Initial state after creation
    // is not signaled.
    enum SignalSource
    {
        SignalSourceNone         = 0, // Semaphore is not pending signal.
        SignalSourceQueue        = 1, // Semaphore is pending signal from a queue operation.
        SignalSourceAcquireImage = 2  // Semaphore is pending signal from a swapchain acquire image operation.
    };

    SignalSource signaled{ SignalSourceNone };
    VkDevice     device{ VK_NULL_HANDLE };
};

struct CommandPoolWrapper;
struct CommandBufferWrapper : public HandleWrapper<VkCommandBuffer>
{
    VkCommandBufferLevel       level{ VK_COMMAND_BUFFER_LEVEL_PRIMARY };
    util::MemoryOutputStream   command_data;
    std::set<format::HandleId> command_handles[CommandHandleType::NumHandleTypes];

    // Pool from which command buffer was allocated. The command buffer must be removed from the pool's allocation list
    // when destroyed.
    CommandPoolWrapper* pool{ nullptr };

    // Image layout info tracked for image barriers recorded to the command buffer. To be updated on calls to
    // vkCmdPipelineBarrier and vkCmdEndRenderPass and applied to the image wrapper on calls to vkQueueSubmit. To be
    // transferred from secondary command buffers to primary command buffers on calls to vkCmdExecuteCommands.
    std::unordered_map<VkImage, VkImageLayout> pending_layouts;

    // Active query info for queries that have been recorded to this command buffer, which will be transfered to the
    // QueryPoolWrapper as pending queries when the command buffer is submitted to a queue.
    std::unordered_map<VkQueryPool, std::unordered_map<uint32_t, QueryInfo>> recorded_queries;

    // Render pass object tracking for processing image layout transitions. Render pass and framebuffer values
    // for the active render pass instance will be set on calls to vkCmdBeginRenderPass and will be used to update the
    // pending image layout on calls to vkCmdEndRenderPass.
    VkRenderPass  active_render_pass{ VK_NULL_HANDLE };
    VkFramebuffer render_pass_framebuffer{ VK_NULL_HANDLE };
};

struct DeviceMemoryWrapper : public HandleWrapper<VkDeviceMemory>
{
    uint32_t         memory_type_index{ std::numeric_limits<uint32_t>::max() };
    VkDeviceSize     allocation_size{ 0 };
    VkDevice         map_device{ VK_NULL_HANDLE };
    const void*      mapped_data{ nullptr };
    VkDeviceSize     mapped_offset{ 0 };
    VkDeviceSize     mapped_size{ 0 };
    VkMemoryMapFlags mapped_flags{ 0 };
};

struct QueryPoolWrapper : public HandleWrapper<VkQueryPool>
{
    VkDevice               device{ VK_NULL_HANDLE };
    VkQueryType            query_type{};
    std::vector<QueryInfo> pending_queries;
};

struct PipelineLayoutWrapper : public HandleWrapper<VkPipelineLayout>
{
    // Creation info for objects used to create the pipeline layout, which may have been destroyed after pipeline layout
    // creation.
    std::shared_ptr<PipelineLayoutDependencies> layout_dependencies;
};

struct RenderPassWrapper : public HandleWrapper<VkRenderPass>
{
    // Final image attachment layouts to be used for processing image layout transitions after calls to
    // vkCmdEndRenderPass.
    std::vector<VkImageLayout> attachment_final_layouts;
};

struct PipelineWrapper : public HandleWrapper<VkPipeline>
{
    // Creation info for objects used to create the pipeline, which may have been destroyed after pipeline creation.
    std::vector<ShaderModuleInfo> shader_modules;

    // TODO: This only requires the unique sequence number once handles are fully wrapped.
    VkRenderPass      render_pass{ VK_NULL_HANDLE };
    format::HandleId  render_pass_id{ 0 };
    format::ApiCallId render_pass_create_call_id{ format::ApiCallId::ApiCall_Unknown };
    CreateParameters  render_pass_create_parameters;

    // TODO: This only requires the unique sequence number once handles are fully wrapped.
    VkPipelineLayout                            layout{ VK_NULL_HANDLE };
    format::HandleId                            layout_id{ 0 };
    format::ApiCallId                           layout_create_call_id{ format::ApiCallId::ApiCall_Unknown };
    CreateParameters                            layout_create_parameters;
    std::shared_ptr<PipelineLayoutDependencies> layout_dependencies;

    // TODO: Base pipeline
    // TODO: Pipeline cache
};

struct DescriptorSetLayoutWrapper : public HandleWrapper<VkDescriptorSetLayout>
{
    std::vector<DescriptorBindingInfo> binding_info;
};

struct DescriptorPoolWrapper;
struct DescriptorSetWrapper : public HandleWrapper<VkDescriptorSet>
{
    VkDevice device{ VK_NULL_HANDLE };

    // Map for descriptor binding index to array of descriptor info.
    std::unordered_map<uint32_t, DescriptorInfo> bindings;

    // Pool from which set was allocated. The set must be removed from the pool's allocation list when destroyed.
    DescriptorPoolWrapper* pool{ nullptr };
};

struct DescriptorPoolWrapper : public HandleWrapper<VkDescriptorPool>
{
    // Track descriptor set info, which must be destroyed on descriptor pool reset.
    std::unordered_map<VkDescriptorSet, DescriptorSetWrapper*> allocated_sets;
};

struct CommandPoolWrapper : public HandleWrapper<VkCommandPool>
{
    uint32_t queue_family_index{ 0 };

    // Track command buffer info, which must be destroyed on command pool reset.
    std::unordered_map<VkCommandBuffer, CommandBufferWrapper*> allocated_buffers;
};

struct SurfaceKHRWrapper : public HandleWrapper<VkSurfaceKHR>
{
    // Track results from calls to vkGetPhysicalDeviceSurfaceSupportKHR to write to the state snapshot after surface
    // creation. The call is only written to the state snapshot if it was previously called by the application.
    std::unordered_map<VkPhysicalDevice, std::unordered_map<uint32_t, VkBool32>> surface_support;
    std::unordered_map<VkPhysicalDevice, VkSurfaceCapabilitiesKHR>               surface_capabilities;
    std::unordered_map<VkPhysicalDevice, std::vector<VkSurfaceFormatKHR>>        surface_formats;
    std::unordered_map<VkPhysicalDevice, std::vector<VkPresentModeKHR>>          surface_present_modes;
};

struct SwapchainKHRWrapper : public HandleWrapper<VkSwapchainKHR>
{
    VkDevice                       device{ VK_NULL_HANDLE };
    VkSurfaceKHR                   surface{ VK_NULL_HANDLE };
    uint32_t                       queue_family_index{ 0 };
    VkFormat                       format{ VK_FORMAT_UNDEFINED };
    VkExtent3D                     extent{ 0, 0, 0 };
    uint32_t                       array_layers{ 0 };
    uint32_t                       last_presented_image{ std::numeric_limits<uint32_t>::max() };
    std::vector<ImageAcquiredInfo> image_acquired_info;
    std::vector<ImageWrapper*>     images;
};

struct ObjectTableNVXWrapper : public HandleWrapper<VkObjectTableNVX>
{
    // TODO: Determine what additional state tracking is needed.
};

struct AccelerationStructureNVWrapper : public HandleWrapper<VkAccelerationStructureNV>
{
    // TODO: Determine what additional state tracking is needed.
};

GFXRECON_END_NAMESPACE(encode)
GFXRECON_END_NAMESPACE(gfxrecon)

#endif // GFXRECON_ENCODE_VULKAN_HANDLE_WRAPPERS_H

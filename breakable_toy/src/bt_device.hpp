#ifndef BT_DEVICE_HPP
#define BT_DEVICE_HPP

#include "bt_window.hpp"

#include <glad/vulkan.h>

#include <string>
#include <vector>

namespace bt {
struct swapchain_support_details {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

struct queue_family_indices {
    uint32_t graphics;
    uint32_t present;
    bool graphics_has_value = false;
    bool present_has_value = false;
    bool isComplete() { return graphics_has_value && present_has_value; }
};

class bt_device {
  public:
#ifdef NDEBUG
    const bool enable_validation_layers = false;
#else
    const bool enable_validation_layers = true;
#endif

    bt_device(bt_window& window);
    bt_device(const bt_device&) = delete;
    bt_device(bt_device&&) = delete;
    ~bt_device();

    void operator=(const bt_device&) = delete;
    bt_device& operator=(bt_device&&) = delete;

    VkAllocationCallbacks* allocator() { return allocator_; }
    VkCommandPool command_pool() { return command_pool_; }
    VkDevice device() { return device_; }
    VkSurfaceKHR surface() { return surface_; }
    VkQueue graphics_queue() { return graphics_queue_; }
    VkQueue present_queue() { return present_queue_; }

    swapchain_support_details swapchain_support()
    {
        return query_swapchain_support(physical_device);
    }

    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);

    queue_family_indices find_physical_queue_families()
    {
        return find_queue_families(physical_device);
    }

    VkFormat find_supported_formats(const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features);

    void create_buffer(VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& buffer_memory);

    VkCommandBuffer begin_single_time_commands();
    void end_single_time_commands(VkCommandBuffer command_buffer);
    void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    void copy_buffer_to_image(
        VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layer_count);

    void create_image_with_info(const VkImageCreateInfo& image_info,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& image_memory);

    VkPhysicalDeviceProperties properties;

  private:
    void create_instance();
    void setup_debug_messenger();
    void create_surface();
    void pick_physical_device();
    void create_logical_device();
    void create_command_pool();

    bool is_device_suitable(VkPhysicalDevice device);
    std::vector<const char*> get_required_instance_extensions();
    std::vector<const char*> get_required_device_extensions(VkPhysicalDevice device);
    bool check_validation_layer_support();
    queue_family_indices find_queue_families(VkPhysicalDevice device);
    void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);
    void check_instance_extension_support();
    bool check_device_extension_support(VkPhysicalDevice device);
    swapchain_support_details query_swapchain_support(VkPhysicalDevice device);

    VkAllocationCallbacks* allocator_ = nullptr;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    bt_window& window;
    VkCommandPool command_pool_;
    VkDevice device_;
    VkSurfaceKHR surface_;
    VkQueue graphics_queue_;
    VkQueue present_queue_;

    const std::vector<const char*> required_validation_layers = { "VK_LAYER_KHRONOS_validation" };
};
} // namespace bt

#endif // BT_DEVICE_HPP

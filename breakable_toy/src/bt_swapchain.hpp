#ifndef BT_SWAPCHAIN_HPP
#define BT_SWAPCHAIN_HPP

#include "bt_device.hpp"

#include <glad/vulkan.h>

#include <string>
#include <vector>

namespace bt {
class bt_swapchain {
  public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    bt_swapchain(bt_device& device, VkExtent2D window_extent);
    ~bt_swapchain();

    bt_swapchain(const bt_swapchain&) = delete;
    void operator=(const bt_swapchain&) = delete;

    VkFramebuffer framebuffer(int index) { return swapchain_framebuffers[index]; }
    VkRenderPass render_pass() { return render_pass_; }
    VkImageView image_view(int index) { return swapchain_image_views[index]; }
    size_t image_count() { return swapchain_images.size(); }
    VkFormat swapchain_image_format() { return swapchain_image_format_; }
    VkExtent2D swapchain_extent() { return swapchain_extent_; }
    uint32_t width() { return swapchain_extent_.width; }
    uint32_t height() { return swapchain_extent_.height; }

    float extent_aspect_ratio()
    {
        return static_cast<float>(swapchain_extent_.width)
            / static_cast<float>(swapchain_extent_.height);
    }

    VkFormat find_depth_format();
    VkResult acquire_next_image(uint32_t* imageIndex);
    VkResult submit_command_buffers(const VkCommandBuffer* buffers, uint32_t* image_index);

  private:
    void create_swapchain();
    void create_image_views();
    void create_depth_resources();
    void create_render_pass();
    void create_framebuffers();
    void create_sync_objects();

    VkSurfaceFormatKHR choose_swap_surface_format(
        const std::vector<VkSurfaceFormatKHR>& available_formats);
    VkPresentModeKHR choose_swap_present_mode(
        const std::vector<VkPresentModeKHR>& available_present_modes);
    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

    VkFormat swapchain_image_format_;
    VkExtent2D swapchain_extent_;
    std::vector<VkFramebuffer> swapchain_framebuffers;
    VkRenderPass render_pass_;
    std::vector<VkImage> depth_images;
    std::vector<VkDeviceMemory> depth_image_memorys;
    std::vector<VkImageView> depth_image_views;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    bt_device& device;
    VkExtent2D window_extent;
    VkSwapchainKHR swapchain;
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;
    std::vector<VkFence> images_in_flight;
    size_t current_frame = 0;
};
} // namespace bt

#endif // BT_SWAPCHAIN_HPP

#include "bt_swapchain.hpp"

#include "bt_logger.hpp"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace bt {
bt_swapchain::bt_swapchain(bt_device& device, VkExtent2D extent) :
    device { device },
    window_extent { extent }
{
    create_swapchain();
    create_image_views();
    create_render_pass();
    create_depth_resources();
    create_framebuffers();
    create_sync_objects();
}

bt_swapchain::~bt_swapchain()
{
    auto* allocator = device.allocator();

    for (auto image_view : swapchain_image_views) {
        vkDestroyImageView(device.device(), image_view, allocator);
    }
    swapchain_image_views.clear();

    if (swapchain != nullptr) {
        vkDestroySwapchainKHR(device.device(), swapchain, allocator);
        swapchain = nullptr;
    }

    for (int i = 0; i < depth_images.size(); i++) {
        vkDestroyImageView(device.device(), depth_image_views[i], allocator);
        vkDestroyImage(device.device(), depth_images[i], allocator);
        vkFreeMemory(device.device(), depth_image_memorys[i], allocator);
    }

    for (auto framebuffer : swapchain_framebuffers) {
        vkDestroyFramebuffer(device.device(), framebuffer, allocator);
    }

    vkDestroyRenderPass(device.device(), render_pass_, allocator);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device.device(), render_finished_semaphores[i], allocator);
        vkDestroySemaphore(device.device(), image_available_semaphores[i], allocator);
        vkDestroyFence(device.device(), in_flight_fences[i], allocator);
    }
}

VkResult bt_swapchain::acquire_next_image(uint32_t* image_index)
{
    vkWaitForFences(device.device(),
        1,
        &in_flight_fences[current_frame],
        VK_TRUE,
        std::numeric_limits<uint64_t>::max());

    VkResult result = vkAcquireNextImageKHR(device.device(),
        swapchain,
        std::numeric_limits<uint64_t>::max(),
        image_available_semaphores[current_frame], // must be a not signaled semaphore
        VK_NULL_HANDLE,
        image_index);

    return result;
}

VkResult bt_swapchain::submit_command_buffers(const VkCommandBuffer* buffers, uint32_t* image_index)
{
    if (images_in_flight[*image_index] != VK_NULL_HANDLE) {
        vkWaitForFences(device.device(), 1, &images_in_flight[*image_index], VK_TRUE, UINT64_MAX);
    }
    images_in_flight[*image_index] = in_flight_fences[current_frame];

    VkSemaphore wait_semaphores[] = { image_available_semaphores[current_frame] };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = buffers;

    VkSemaphore signal_semaphores[] = { render_finished_semaphores[current_frame] };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    vkResetFences(device.device(), 1, &in_flight_fences[current_frame]);
    if (vkQueueSubmit(device.graphics_queue(), 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer");
    }

    VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapchains[] = { swapchain };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;

    present_info.pImageIndices = image_index;

    auto result = vkQueuePresentKHR(device.present_queue(), &present_info);

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}

void bt_swapchain::create_swapchain()
{
    swapchain_support_details swapchain_support = device.swapchain_support();

    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swapchain_support.formats);
    VkPresentModeKHR present_mode = choose_swap_present_mode(swapchain_support.present_modes);
    VkExtent2D extent = choose_swap_extent(swapchain_support.capabilities);

    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0
        && image_count > swapchain_support.capabilities.maxImageCount) {
        image_count = swapchain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    create_info.surface = device.surface();
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    queue_family_indices indices = device.find_physical_queue_families();
    uint32_t queue_family_indices_[] = { indices.graphics, indices.present };

    if (indices.graphics != indices.present) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices_;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    create_info.preTransform = swapchain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;

    create_info.oldSwapchain = VK_NULL_HANDLE;

    if (!vkCreateSwapchainKHR) {
        SPDLOG_CRITICAL("function is nullptr");
    }

    if (vkCreateSwapchainKHR(device.device(), &create_info, device.allocator(), &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain");
    }

    vkGetSwapchainImagesKHR(device.device(), swapchain, &image_count, nullptr);
    swapchain_images.resize(image_count);
    vkGetSwapchainImagesKHR(device.device(), swapchain, &image_count, swapchain_images.data());

    swapchain_image_format_ = surface_format.format;
    swapchain_extent_ = extent;
}

void bt_swapchain::create_image_views()
{
    swapchain_image_views.resize(swapchain_images.size());
    for (size_t i = 0; i < swapchain_images.size(); i++) {
        VkImageViewCreateInfo view_info { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        view_info.image = swapchain_images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = swapchain_image_format_;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device.device(), &view_info, device.allocator(), &swapchain_image_views[i])
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view");
        }
    }
}

void bt_swapchain::create_render_pass()
{
    VkAttachmentDescription depth_attachment {};
    depth_attachment.format = find_depth_format();
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref {};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription color_attachment = {};
    color_attachment.format = swapchain_image_format();
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask
        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstSubpass = 0;
    dependency.dstStageMask
        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo render_pass_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(device.device(), &render_pass_info, device.allocator(), &render_pass_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass");
    }
}

void bt_swapchain::create_framebuffers()
{
    swapchain_framebuffers.resize(image_count());
    for (size_t i = 0; i < image_count(); i++) {
        std::array<VkImageView, 2> attachments = { swapchain_image_views[i], depth_image_views[i] };

        VkExtent2D extent = swapchain_extent();

        VkFramebufferCreateInfo framebuffer_info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebuffer_info.renderPass = render_pass_;
        framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = extent.width;
        framebuffer_info.height = extent.height;
        framebuffer_info.layers = 1;

        if (vkCreateFramebuffer(device.device(), &framebuffer_info, device.allocator(), &swapchain_framebuffers[i])
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer");
        }
    }
}

void bt_swapchain::create_depth_resources()
{
    VkFormat depth_format = find_depth_format();
    VkExtent2D extent = swapchain_extent();

    depth_images.resize(image_count());
    depth_image_memorys.resize(image_count());
    depth_image_views.resize(image_count());

    for (int i = 0; i < depth_images.size(); i++) {
        VkImageCreateInfo image_info { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = extent.width;
        image_info.extent.height = extent.height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.format = depth_format;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.flags = 0;

        device.create_image_with_info(image_info,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            depth_images[i],
            depth_image_memorys[i]);

        VkImageViewCreateInfo view_info { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        view_info.image = depth_images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = depth_format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device.device(), &view_info, device.allocator(), &depth_image_views[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view");
        }
    }
}

void bt_swapchain::create_sync_objects()
{
    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
    images_in_flight.resize(image_count(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device.device(), &semaphore_info, device.allocator(), &image_available_semaphores[i])
                != VK_SUCCESS
            || vkCreateSemaphore(device.device(), &semaphore_info, device.allocator(), &render_finished_semaphores[i])
                != VK_SUCCESS
            || vkCreateFence(device.device(), &fence_info, device.allocator(), &in_flight_fences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame");
        }
    }
}

VkSurfaceFormatKHR bt_swapchain::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
    for (const auto& available_format : available_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM
            && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_format;
        }
    }

    return available_formats[0];
}

VkPresentModeKHR bt_swapchain::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
    // for (const auto& available_present_mode : available_present_modes) {
    //     if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
    //         SPDLOG_DEBUG("Present mode: Mailbox");
    //         return available_present_mode;
    //     }
    // }

    // for (const auto& availablePresentMode : available_present_modes) {
    //     if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
    //         SPDLOG_DEBUG("Present mode: Immediate");
    //         return availablePresentMode;
    //     }
    // }

    SPDLOG_DEBUG("Present mode: V-Sync");
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D bt_swapchain::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actual_extent = window_extent;
        actual_extent.width = std::max(capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actual_extent.width));
        actual_extent.height = std::max(capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actual_extent.height));

        return actual_extent;
    }
}

VkFormat bt_swapchain::find_depth_format()
{
    return device.find_supported_format(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}
} // namespace bt

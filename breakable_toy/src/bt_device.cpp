#include "bt_device.hpp"

#include "bt_logger.hpp"

#include <cstring>
#include <iostream>
#include <set>
#include <unordered_set>

namespace bt {
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    SPDLOG_ERROR("{} - {} - {}",
        callback_data->messageIdNumber,
        callback_data->pMessageIdName,
        callback_data->pMessage);

    return VK_FALSE;
}

VkResult create_debug_utils_messenger_ext(VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* create_info,
    const VkAllocationCallbacks* allocator,
    VkDebugUtilsMessengerEXT* debug_messenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, create_info, allocator, debug_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroy_debug_utils_messenger_ext(
    VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* allocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debug_messenger, allocator);
    }
}

bt_device::bt_device(bt_window& window)
    : window { window }
{
    load_vulkan_function_pointers(nullptr, nullptr, nullptr);
    create_instance();
    load_vulkan_function_pointers(instance, nullptr, nullptr);
    setup_debug_messenger();
    create_surface();
    pick_physical_device();
    load_vulkan_function_pointers(instance, physical_device, nullptr);
    create_logical_device();
    load_vulkan_function_pointers(instance, physical_device, device_);
    create_command_pool();
}

bt_device::~bt_device()
{
    vkDestroyCommandPool(device_, command_pool_, allocator_);
    vkDestroyDevice(device_, allocator_);

    if (enable_validation_layers) {
        destroy_debug_utils_messenger_ext(instance, debug_messenger, allocator_);
    }

    vkDestroySurfaceKHR(instance, surface_, allocator_);
    vkDestroyInstance(instance, allocator_);
}

void bt_device::load_vulkan_function_pointers(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device)
{
    auto glad_vk_version = gladLoaderLoadVulkan(instance, physical_device, device);
    if (!glad_vk_version) {
        throw std::runtime_error("unable to load Vulkan symbols, gladLoad failure");
    }

    auto major = GLAD_VERSION_MAJOR(glad_vk_version);
    auto minor = GLAD_VERSION_MINOR(glad_vk_version);
    SPDLOG_DEBUG("Vulkan version {}.{}", major, minor);
}

void bt_device::create_instance()
{
    if (enable_validation_layers && !check_validation_layer_support()) {
        throw std::runtime_error("validation layers requested, but not available");
    }

    VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pApplicationName = "Breakable Toy App";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "Breakable Toy";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    create_info.pApplicationInfo = &app_info;

    auto extensions = get_required_instance_extensions();
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
    if (enable_validation_layers) {
        create_info.enabledLayerCount = static_cast<uint32_t>(required_validation_layers.size());
        create_info.ppEnabledLayerNames = required_validation_layers.data();

        populate_debug_messenger_create_info(debug_create_info);
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
    } else {
        create_info.enabledLayerCount = 0;
        create_info.pNext = nullptr;
    }

    if (vkCreateInstance(&create_info, allocator_, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance");
    }

    check_instance_extension_support();
}

void bt_device::pick_physical_device()
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    if (device_count == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support");
    }
    SPDLOG_DEBUG("device count: {}", device_count);

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    for (const auto& device : devices) {
        if (is_device_suitable(device)) {
            physical_device = device;
            break;
        }
    }

    if (physical_device == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU");
    }

    vkGetPhysicalDeviceProperties(physical_device, &properties);
    SPDLOG_DEBUG("physical device: {}", properties.deviceName);
}

void bt_device::create_logical_device()
{
    queue_family_indices indices = find_queue_families(physical_device);

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = { indices.graphics, indices.present };

    float queue_priority = 1.0f;
    for (uint32_t queueFamily : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        queue_create_info.queueFamilyIndex = queueFamily;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features = {};
    device_features.samplerAnisotropy = VK_TRUE;

    std::vector<const char*> required_device_extensions = get_required_device_extensions(physical_device);

    VkDeviceCreateInfo create_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = static_cast<uint32_t>(required_device_extensions.size());
    create_info.ppEnabledExtensionNames = required_device_extensions.data();

    if (enable_validation_layers) {
        create_info.enabledLayerCount = static_cast<uint32_t>(required_validation_layers.size());
        create_info.ppEnabledLayerNames = required_validation_layers.data();
    } else {
        create_info.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physical_device, &create_info, allocator_, &device_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device");
    }

    vkGetDeviceQueue(device_, indices.graphics, 0, &graphics_queue_);
    vkGetDeviceQueue(device_, indices.present, 0, &present_queue_);
}

void bt_device::create_command_pool()
{
    queue_family_indices queue_family_indices = find_physical_queue_families();

    VkCommandPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_info.queueFamilyIndex = queue_family_indices.graphics;
    pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device_, &pool_info, allocator_, &command_pool_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool");
    }
}

void bt_device::create_surface() { window.create_window_surface(instance, &surface_, allocator_); }

bool bt_device::is_device_suitable(VkPhysicalDevice device)
{
    queue_family_indices indices = find_queue_families(device);

    bool extensions_supported = check_device_extension_support(device);

    bool swapchain_adequate = false;
    if (extensions_supported) {
        swapchain_support_details swapchain_support = query_swapchain_support(device);
        swapchain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
    }

    VkPhysicalDeviceFeatures supported_features;
    vkGetPhysicalDeviceFeatures(device, &supported_features);

    return indices.isComplete() && extensions_supported && swapchain_adequate && supported_features.samplerAnisotropy;
}

void bt_device::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
    create_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    create_info.messageSeverity
        = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_messenger_callback;
    create_info.pUserData = nullptr;
}

void bt_device::setup_debug_messenger()
{
    if (!enable_validation_layers) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT create_info;
    populate_debug_messenger_create_info(create_info);
    if (create_debug_utils_messenger_ext(instance, &create_info, allocator_, &debug_messenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger");
    }
}

bool bt_device::check_validation_layer_support()
{
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (const char* layer_name : required_validation_layers) {
        bool layer_found = false;

        for (const auto& layer_properties : available_layers) {
            if (strcmp(layer_name, layer_properties.layerName) == 0) {
                layer_found = true;
                break;
            }
        }

        if (!layer_found) {
            return false;
        }
    }

    return true;
}

std::vector<const char*> bt_device::get_required_instance_extensions()
{
    uint32_t extension_count = 0;
    const char** extensions;
    extensions = glfwGetRequiredInstanceExtensions(&extension_count);

    std::vector<const char*> required_extensions(extensions, extensions + extension_count);

#if __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_MAC
#if TARGET_CPU_ARM64
    required_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#elif TARGET_CPU_X86_64
    // Is there an extension needed here?
#endif
#endif
#endif

    if (enable_validation_layers) {
        required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return required_extensions;
}

std::vector<const char*> bt_device::get_required_device_extensions(VkPhysicalDevice device)
{
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::vector<const char*> required_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    bool requires_portability_subset = std::any_of(available_extensions.begin(),
        available_extensions.end(),
        [](const auto& available) { return strcmp(available.extensionName, "VK_KHR_portability_subset") == 0; });

    if (requires_portability_subset) {
        required_extensions.push_back("VK_KHR_portability_subset");
    }

    return required_extensions;
}

void bt_device::check_instance_extension_support()
{
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

    SPDLOG_DEBUG("available extensions:");
    std::unordered_set<std::string> available;
    for (const auto& extension : extensions) {
        SPDLOG_DEBUG("\t{}", extension.extensionName);
        available.insert(extension.extensionName);
    }

    SPDLOG_DEBUG("required extensions:");
    auto required_extensions = get_required_instance_extensions();
    for (const auto& required : required_extensions) {
        SPDLOG_DEBUG("\t{}", required);
        if (available.find(required) == available.end()) {
            throw std::runtime_error("missing required extension");
        }
    }
}

bool bt_device::check_device_extension_support(VkPhysicalDevice device)
{
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::vector<const char*> required = get_required_device_extensions(device);
    std::set<std::string> required_extensions(required.begin(), required.end());

    for (const auto& extension : available_extensions) {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

queue_family_indices bt_device::find_queue_families(VkPhysicalDevice device)
{
    queue_family_indices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    int i = 0;
    for (const auto& queue_family : queue_families) {
        if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics = i;
            indices.graphics_has_value = true;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support);
        if (queue_family.queueCount > 0 && present_support) {
            indices.present = i;
            indices.present_has_value = true;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

swapchain_support_details bt_device::query_swapchain_support(VkPhysicalDevice device)
{
    swapchain_support_details details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);

    if (format_count != 0) {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats.data());
    }

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);

    if (present_mode_count != 0) {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, details.present_modes.data());
    }

    return details;
}

VkFormat bt_device::find_supported_format(
    const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format");
}

uint32_t bt_device::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type");
}

void bt_device::create_buffer(VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& buffer_memory)
{
    VkBufferCreateInfo buffer_info { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_, &buffer_info, allocator_, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer");
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device_, buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device_, &alloc_info, allocator_, &buffer_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory");
    }

    vkBindBufferMemory(device_, buffer, buffer_memory, 0);
}

VkCommandBuffer bt_device::begin_single_time_commands()
{
    VkCommandBufferAllocateInfo alloc_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = command_pool_;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device_, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);
    return command_buffer;
}

void bt_device::end_single_time_commands(VkCommandBuffer command_buffer)
{
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(graphics_queue_, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue_);

    vkFreeCommandBuffers(device_, command_pool_, 1, &command_buffer);
}

void bt_device::copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    VkCommandBuffer command_buffer = begin_single_time_commands();

    VkBufferCopy copy_region {};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy_region);

    end_single_time_commands(command_buffer);
}

void bt_device::copy_buffer_to_image(
    VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layer_count)
{
    VkCommandBuffer command_buffer = begin_single_time_commands();

    VkBufferImageCopy region {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = layer_count;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    end_single_time_commands(command_buffer);
}

void bt_device::create_image_with_info(
    const VkImageCreateInfo& image_info, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory)
{
    if (vkCreateImage(device_, &image_info, allocator_, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image");
    }

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device_, image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device_, &alloc_info, allocator_, &image_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory");
    }

    if (vkBindImageMemory(device_, image, image_memory, 0) != VK_SUCCESS) {
        throw std::runtime_error("failed to bind image memory");
    }
}
} // namespace bt

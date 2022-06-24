#ifndef BT_MODEL_HPP
#define BT_MODEL_HPP

#include "bt_device.hpp"
#include "bt_maths.hpp"

#include <glad/vulkan.h>

#include <vector>

namespace bt {
class bt_model {
  public:
    struct vertex {
        glm::vec2 position;
        glm::vec3 color;

        static std::vector<VkVertexInputBindingDescription> binding_descriptions();
        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions();
    };

    bt_model(bt_device& device, const std::vector<vertex>& vertices);
    bt_model(const bt_model&) = delete;
    ~bt_model();

    bt_model& operator=(const bt_model&) = delete;

    void bind(VkCommandBuffer command_buffer);
    void draw(VkCommandBuffer command_buffer);

  private:
    void create_vertex_buffers(const std::vector<vertex>& vertices);

    bt_device& device_;
    VkBuffer vertex_buffer_;
    VkDeviceMemory vertex_buffer_memory_;
    uint32_t vertex_count_;
};
} // namespace bt

#endif // BT_MODEL_HPP

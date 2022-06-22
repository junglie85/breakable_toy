#include "bt_model.hpp"

#include <cassert>
#include <cstring>

namespace bt {
std::vector<VkVertexInputBindingDescription> bt_model::vertex::binding_descriptions()
{
    std::vector<VkVertexInputBindingDescription> descriptions(1);

    descriptions[0].binding = 0;
    descriptions[0].stride = sizeof(vertex);
    descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return descriptions;
}

std::vector<VkVertexInputAttributeDescription> bt_model::vertex::attribute_descriptions()
{
    std::vector<VkVertexInputAttributeDescription> descriptions(2);

    descriptions[0].binding = 0;
    descriptions[0].location = 0;
    descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    descriptions[0].offset = offsetof(vertex, position);

    descriptions[1].binding = 0;
    descriptions[1].location = 1;
    descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[1].offset = offsetof(vertex, color);

    return descriptions;
}

bt_model::bt_model(bt_device& device, const std::vector<vertex>& vertices) :
    device_(device)
{
    create_vertex_buffers(vertices);
}

bt_model::~bt_model()
{
    vkDestroyBuffer(device_.device(), vertex_buffer_, device_.allocator());
    vkFreeMemory(device_.device(), vertex_buffer_memory_, device_.allocator());
}

void bt_model::bind(VkCommandBuffer command_buffer)
{
    VkBuffer buffers[] = { vertex_buffer_ };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, buffers, offsets);
}

void bt_model::draw(VkCommandBuffer command_buffer) { vkCmdDraw(command_buffer, vertex_count_, 1, 0, 0); }

void bt_model::create_vertex_buffers(const std::vector<vertex>& vertices)
{
    vertex_count_ = static_cast<uint32_t>(vertices.size());
    assert(vertex_count_ >= 3 && "Vertex count must be at least 3");
    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertex_count_;
    device_.create_buffer(buffer_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertex_buffer_,
        vertex_buffer_memory_);

    void* data;
    vkMapMemory(device_.device(), vertex_buffer_memory_, 0, buffer_size, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
    vkUnmapMemory(device_.device(), vertex_buffer_memory_);
}
} // namespace bt

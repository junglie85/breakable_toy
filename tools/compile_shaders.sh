#!/usr/bin/env bash

TOOLS_DIR="$(cd "$(dirname "$BASH_SOURCE")"; pwd)"
SHADER_DIR="${TOOLS_DIR}/../breakable_toy/shaders"

GLSLC=${VULKAN_SDK}/bin/glslc

${GLSLC} -fshader-stage=vert -o "${SHADER_DIR}/simple_shader.vert.spv" "${SHADER_DIR}/simple_shader.vert.glsl"
${GLSLC} -fshader-stage=frag -o "${SHADER_DIR}/simple_shader.frag.spv" "${SHADER_DIR}/simple_shader.frag.glsl"

#pragma once

#include "engine/rendering/resources/Mesh.hpp"
#include <string>
#include <memory>
#include <unordered_map>

namespace vulkan_engine::rendering
{
    // ============================================================================
    // ObjLoader - Simple OBJ file loader
    // Supports: vertices, normals, texture coordinates, faces
    // Does NOT support: materials (use our Material system instead),
    //                   smoothing groups, object groups
    // ============================================================================
    class ObjLoader
    {
        public:
            ObjLoader()  = default;
            ~ObjLoader() = default;

            // Load OBJ file from path, returns CPU-side mesh data
            MeshData load(const std::string& path);

            // Check if file exists and is readable
            bool can_load(const std::string& path) const;

            // Get last error message
            const std::string& last_error() const { return last_error_; }

        private:
            std::string last_error_;

            // Helper to parse face indices (handles v/vt/vn format)
            struct FaceIndex
            {
                int v  = 0; // vertex index
                int vt = 0; // texture coord index
                int vn = 0; // normal index
            };

            FaceIndex parse_face_index(const std::string& token) const;
    };
} // namespace vulkan_engine::rendering
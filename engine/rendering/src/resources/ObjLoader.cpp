#include "engine/rendering/resources/ObjLoader.hpp"
#include "engine/core/utils/Logger.hpp"
#include "engine/platform/filesystem/PathUtils.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <array>
#include <cmath>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <codecvt>
#include <locale>
#endif

namespace vulkan_engine::rendering
{
    MeshData ObjLoader::load(const std::string& path)
    {
        MeshData result;
        result.name = path;

        auto file = core::PathUtils::open_input_file(path);
        if (!file.is_open())
        {
            last_error_ = "Failed to open file: " + path;
            logger::error(last_error_);
            return result;
        }

        // Temporary storage for raw OBJ data
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texcoords;

        // For index deduplication
        std::unordered_map<std::string, uint32_t> index_map;

        std::string line;
        while (std::getline(file, line))
        {
            // Trim leading whitespace
            size_t start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            line = line.substr(start);

            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') continue;

            std::istringstream iss(line);
            std::string        prefix;
            iss >> prefix;

            if (prefix == "v") // Vertex position
            {
                glm::vec3 pos;
                iss >> pos.x >> pos.y >> pos.z;
                positions.push_back(pos);
            }
            else if (prefix == "vn") // Vertex normal
            {
                glm::vec3 norm;
                iss >> norm.x >> norm.y >> norm.z;
                normals.push_back(norm);
            }
            else if (prefix == "vt") // Texture coordinate
            {
                glm::vec2 tex;
                iss >> tex.x >> tex.y;
                texcoords.push_back(tex);
            }
            else if (prefix == "f") // Face
            {
                std::vector<std::string> face_tokens;
                std::string              token;
                while (iss >> token)
                {
                    face_tokens.push_back(token);
                }

                // Triangulate face (handle quads and polygons)
                // For simplicity, use fan triangulation
                for (size_t i = 2; i < face_tokens.size(); ++i)
                {
                    // Triangle: 0, i-1, i
                    const std::array<size_t, 3> indices = {0, i - 1, i};
                    for (size_t idx : indices)
                    {
                        const auto& token_str = face_tokens[idx];

                        // Check if we've seen this vertex before
                        auto it = index_map.find(token_str);
                        if (it != index_map.end())
                        {
                            result.indices.push_back(it->second);
                        }
                        else
                        {
                            // Parse face index
                            FaceIndex fi = parse_face_index(token_str);

                            // Build vertex
                            MeshVertex vertex;

                            // Position (required)
                            if (fi.v > 0 && fi.v <= static_cast<int>(positions.size()))
                            {
                                vertex.position = positions[fi.v - 1];
                            }
                            else if (fi.v < 0 && -fi.v <= static_cast<int>(positions.size()))
                            {
                                vertex.position = positions[positions.size() + fi.v];
                            }

                            // Normal
                            if (fi.vn > 0 && fi.vn <= static_cast<int>(normals.size()))
                            {
                                vertex.normal = normals[fi.vn - 1];
                            }
                            else if (fi.vn < 0 && -fi.vn <= static_cast<int>(normals.size()))
                            {
                                vertex.normal = normals[normals.size() + fi.vn];
                            }

                            // Texture coordinate
                            if (fi.vt > 0 && fi.vt <= static_cast<int>(texcoords.size()))
                            {
                                vertex.uv = texcoords[fi.vt - 1];
                            }
                            else if (fi.vt < 0 && -fi.vt <= static_cast<int>(texcoords.size()))
                            {
                                vertex.uv = texcoords[texcoords.size() + fi.vt];
                            }

                            // Generate color based on position for visual variety
                            vertex.color = glm::vec3(
                                                     0.5f + 0.5f * std::sin(vertex.position.x * 2.0f),
                                                     0.5f + 0.5f * std::sin(vertex.position.y * 2.0f),
                                                     0.5f + 0.5f * std::sin(vertex.position.z * 2.0f));

                            // If no normal was provided, generate a default one
                            if (glm::length(vertex.normal) < 0.001f)
                            {
                                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                            }

                            uint32_t new_index = static_cast<uint32_t>(result.vertices.size());
                            result.vertices.push_back(vertex);
                            result.indices.push_back(new_index);
                            index_map[token_str] = new_index;
                        }
                    }
                }
            }
        }

        file.close();

        // Validate mesh data
        bool has_invalid = false;
        for (size_t i = 0; i < result.vertices.size(); ++i)
        {
            const auto& v = result.vertices[i];
            if (std::isnan(v.position.x) || std::isnan(v.position.y) || std::isnan(v.position.z))
            {
                logger::error("Vertex " + std::to_string(i) + " has NaN position");
                has_invalid = true;
            }
        }

        // Check index bounds
        for (size_t i = 0; i < result.indices.size(); ++i)
        {
            if (result.indices[i] >= result.vertices.size())
            {
                logger::error("Index " + std::to_string(i) + " out of bounds: " +
                              std::to_string(result.indices[i]) + " >= " +
                              std::to_string(result.vertices.size()));
                has_invalid = true;
            }
        }

        // Show first few vertices for debugging
        if (!result.vertices.empty())
        {
            logger::info("First 3 vertices:");
            size_t count = result.vertices.size() < 3 ? result.vertices.size() : 3;
            for (size_t i = 0; i < count; ++i)
            {
                const auto& v = result.vertices[i];
                logger::info("  v[" + std::to_string(i) + "]: pos=(" +
                             std::to_string(v.position.x) + ", " +
                             std::to_string(v.position.y) + ", " +
                             std::to_string(v.position.z) + ")");
            }
        }

        // Show first few indices for debugging
        if (!result.indices.empty())
        {
            logger::info("First 9 indices (first 3 triangles):");
            std::string idx_str;
            size_t      count = result.indices.size() < 9 ? result.indices.size() : 9;
            for (size_t i = 0; i < count; ++i)
            {
                idx_str += std::to_string(result.indices[i]) + " ";
            }
            logger::info("  " + idx_str);
        }

        logger::info("Loaded OBJ: " + core::PathUtils::to_string(std::filesystem::path(path)) +
                     " - Positions: " + std::to_string(positions.size()) +
                     ", Normals: " + std::to_string(normals.size()) +
                     ", TexCoords: " + std::to_string(texcoords.size()) +
                     " => Vertices: " + std::to_string(result.vertices.size()) +
                     ", Indices: " + std::to_string(result.indices.size()) +
                     (has_invalid ? " [HAS INVALID DATA]" : " [OK]"));

        return result;
    }

    bool ObjLoader::can_load(const std::string& path) const
    {
        auto file = core::PathUtils::open_input_file(path);
        return file.good();
    }

    ObjLoader::FaceIndex ObjLoader::parse_face_index(const std::string& token) const
    {
        FaceIndex result;

        // Parse formats:
        // v
        // v/vt
        // v/vt/vn
        // v//vn

        size_t first_slash = token.find('/');
        if (first_slash == std::string::npos)
        {
            // Just vertex index
            result.v = std::stoi(token);
        }
        else
        {
            // Vertex index
            result.v = std::stoi(token.substr(0, first_slash));

            size_t second_slash = token.find('/', first_slash + 1);
            if (second_slash == std::string::npos)
            {
                // v/vt format
                if (first_slash + 1 < token.size())
                {
                    result.vt = std::stoi(token.substr(first_slash + 1));
                }
            }
            else
            {
                // v/vt/vn or v//vn format
                if (first_slash + 1 < second_slash)
                {
                    result.vt = std::stoi(token.substr(first_slash + 1, second_slash - first_slash - 1));
                }
                if (second_slash + 1 < token.size())
                {
                    result.vn = std::stoi(token.substr(second_slash + 1));
                }
            }
        }

        return result;
    }
} // namespace vulkan_engine::rendering
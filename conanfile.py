from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.build import check_min_cppstd

class VulkanEngineConan(ConanFile):
    name = "vulkan-engine"
    version = "2.0.0"
    license = "MIT"
    author = "Vulkan Engine Team"
    url = "https://github.com/your-username/vulkan-engine"
    description = "Modern C++ Vulkan rendering engine with Render Graph support"
    topics = ("vulkan", "graphics", "rendering", "c++20", "render-graph")
    
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_render_graph": [True, False],
        "with_async_loading": [True, False],
        "with_hot_reload": [True, False],
        "with_tests": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_render_graph": True,
        "with_async_loading": True,
        "with_hot_reload": True,
        "with_tests": False,
    }
    
    exports_sources = "CMakeLists.txt", "src/*", "include/*", "shaders/*"
    
    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC
    
    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
    
    def layout(self):
        cmake_layout(self)
    
    def requirements(self):
        # Core graphics libraries
        # Note: Vulkan SDK is expected to be installed system-wide
        self.requires("glfw/3.3.8")
        self.requires("glm/0.9.9.8")
        self.requires("stb/cci.20230920")
        self.requires("nlohmann_json/3.11.2")

        # Optional dependencies
        if self.options.with_async_loading:
            self.requires("taskflow/3.6.0")
        
        # Shader reflection for hot reload
        if self.options.with_hot_reload:
            self.requires("spirv-tools/1.3.268.0")

        # Development tools
        if self.options.with_tests:
            self.requires("catch2/3.4.0")
            self.requires("benchmark/1.8.2")
    
    def validate(self):
        check_min_cppstd(self, "20")
    
    def generate(self):
        # Generate CMake toolchain
        tc = CMakeToolchain(self)
        # Configure CMake options
        tc.variables["VULKAN_ENGINE_USE_RENDER_GRAPH"] = self.options.with_render_graph
        tc.variables["VULKAN_ENGINE_USE_ASYNC_LOADING"] = self.options.with_async_loading
        tc.variables["VULKAN_ENGINE_USE_HOT_RELOAD"] = self.options.with_hot_reload
        tc.variables["VULKAN_ENGINE_BUILD_TESTS"] = False
        tc.variables["VULKAN_ENGINE_BUILD_EXAMPLES"] = True
        tc.generate()
        
        # Generate CMake find modules for dependencies
        from conan.tools.cmake import CMakeDeps
        deps = CMakeDeps(self)
        deps.generate()
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
    
    def package(self):
        cmake = CMake(self)
        cmake.install()
    
    def package_info(self):
        self.cpp_info.libs = ["VulkanEngine"]
        self.cpp_info.set_property("cmake_target_name", "VulkanEngine::VulkanEngine")
        
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs = ["dl", "pthread"]
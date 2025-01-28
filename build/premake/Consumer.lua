-- Function to recursively delete a directory
function delete_directory(dir)
    if os.isdir(dir) then
        os.rmdir(dir)
    end
end

-- Delete the directory '../vs2022' before generating files
delete_directory("../vs2022Consumer")

-- premake5.lua
workspace "Consumer"
    configurations { "Debug", "Release" }
    architecture "x64"

    -- Set the location of generated Visual Studio files
    location "../vs2022Consumer"

    -- Use the latest C++ standard
    cppdialect "C++latest"

    flags { "MultiProcessorCompile" }

    -- Platform-specific settings
    filter "system:windows"
        systemversion "latest"
    filter {}

project "Consumer"
    kind "ConsoleApp"
    language "C++"

    debugdir "../../"

    -- Specify the directories for source and include files
    files {
        "../../src/Consumer.cpp",
        "../../include/**.h",
        "../../extern/cpputils/include/**.h",
        "../../extern/glh/include/**.h",
        "../../extern/glh/src/**.cpp",
        "../../extern/glh/src/**.c"
    }

    removefiles {
        "../../extern/glh/include/glad/glx.h",
        "../../extern/glh/src/glad/glx.c"
    }

    includedirs {
        "../../include",
        "../../extern/cpputils/include",
        "../../extern/glh/include",
        "../../extern/glh/include/dearimgui"
    }

    libdirs {
        "../../lib"
    }

    links {
        "glfw3.lib",
    }

    -- Output directories for build files
    targetdir "../../bin/%{cfg.buildcfg}"
    objdir "../../bin/%{cfg.buildcfg}"

    -- Debug and Release-specific settings
    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter {}
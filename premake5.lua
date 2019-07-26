workspace "tutorial_opengl"
    startproject "uniform"

    configurations
    {
        "Debug",
        "Release"
    }

    platforms
    {
        "Win64"
    }

    filter "platforms:Win64"
        system "Windows"
        architecture "x86_64"

outputdir = "%{cfg.platform}%{cfg.buildcfg}/%{prj.name}"

include "3rd/glfw.lua"
include "3rd/glad.lua"
include "3rd/glm.lua"

project "uniform"
    location "uniform"
    kind "ConsoleApp"
    language "C++"

    debugdir "resources"
    targetdir ("bin/" .. outputdir)
	objdir ("bin/obj/" .. outputdir)

    use_glfw()
    use_glad()
    use_glm()

    files
    {
        "%{prj.name}/**.h",
        "%{prj.name}/**.cpp"
    }

    filter "system:windows"
        cppdialect "C++17"
        staticruntime "On"
        systemversion "latest"

    filter "configurations:Debug*"
        symbols "On"

    filter "configurations:Release*"
        optimize "On"
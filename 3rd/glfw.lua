function use_glfw()
    includedirs "3rd/GLFW/include"
    links "glfw"
end

project "glfw"
    location "glfw"
    kind "StaticLib"
    language "C"

    targetdir ("../bin/" .. outputdir)
	objdir ("../bin/obj/" .. outputdir)

    files
    {
        "%{prj.name}/src/glfw_config.h",
        "%{prj.name}/include/GLFW/glfw3.h",
        "%{prj.name}/include/GLFW/glfw3native.h",
        "%{prj.name}/src/context.c",
        "%{prj.name}/src/init.c",
        "%{prj.name}/src/input.c",
        "%{prj.name}/src/monitor.c",
        "%{prj.name}/src/vulkan.c",
        "%{prj.name}/src/window.c"
    }

    filter "system:windows"
        buildoptions { "-std=c11", "-lgdi32" }
        systemversion "latest"
        staticruntime "On"

        files
        {
            "%{prj.name}/src/win32_init.c",
            "%{prj.name}/src/win32_joystick.c",
            "%{prj.name}/src/win32_monitor.c",
            "%{prj.name}/src/win32_time.c",
            "%{prj.name}/src/win32_thread.c",
            "%{prj.name}/src/win32_window.c",
            "%{prj.name}/src/wgl_context.c",
            "%{prj.name}/src/egl_context.c",
            "%{prj.name}/src/osmesa_context.c"
        }

        defines
        {
            "_GLFW_WIN32",
            "_CRT_SECURE_NO_WARNINGS"
        }

    filter { "system:windows", "configurations:Release" }
        buildoptions "/MT"
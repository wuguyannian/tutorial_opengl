function use_glad()
    includedirs "3rd/glad/include"
    links "glad"
end

project "glad"
    location "glad"
    kind "StaticLib"
    language "C"
    staticruntime "on"

    targetdir ("../bin/" .. outputdir)
	objdir ("../bin/obj/" .. outputdir)

    includedirs "glad/include"

    files
    {
        "glad/include/glad/glad.h",
        "glad/include/KHR/khrplatform.h",
        "glad/src/glad.c"
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        symbols "on"

    filter "configurations:Release"
        symbols "on"
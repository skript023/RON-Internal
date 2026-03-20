include(FetchContent)

FetchContent_Declare(
    sdk
    GIT_REPOSITORY https://github.com/skript023/ReadyOrNotSDK.git
    GIT_TAG        main
    GIT_PROGRESS TRUE
) 
message("sdk")
FetchContent_GetProperties(sdk)
if(NOT sdk_POPULATED)
    FetchContent_Populate(sdk)

    file(GLOB SRC_SDK
        "${sdk_SOURCE_DIR}/SDK/Basic.cpp"
        "${sdk_SOURCE_DIR}/SDK/CoreUObject_functions.cpp"
        "${sdk_SOURCE_DIR}/SDK/Engine_functions.cpp"
        "${sdk_SOURCE_DIR}/SDK/ReadyOrNot_functions.cpp"
    )

    add_library(sdk STATIC ${SRC_SDK} )
    source_group(TREE ${sdk_SOURCE_DIR} PREFIX "sdk" FILES ${SRC_SDK})
    target_include_directories(sdk PRIVATE "${sdk_SOURCE_DIR}")

    if (MSVC)
        target_compile_options(sdk PRIVATE /bigobj)
    endif()
endif()
set_property(TARGET sdk PROPERTY CXX_STANDARD 23)
include(FetchContent)

add_compile_definitions(CXX_FORMAT_SUPPORT)

message("AsyncLogger")
FetchContent_Declare(
    AsyncLogger
    GIT_REPOSITORY https://github.com/skript023/AsyncLogger.git
    GIT_TAG master
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(AsyncLogger)

set_property(TARGET AsyncLogger PROPERTY CXX_STANDARD 23)

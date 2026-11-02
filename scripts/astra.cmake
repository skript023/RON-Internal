include(FetchContent)

message("Fetching Astra UI")
FetchContent_Declare(
    astra
    GIT_REPOSITORY https://github.com/skript023/astra-ui.git
    GIT_TAG 06253123886fdcc10d944659a1fc57c14d54af2b
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(astra)

include(FetchContent)

message("Fetching Astra UI")
FetchContent_Declare(
    astra
    GIT_REPOSITORY https://github.com/skript023/astra-ui.git
    GIT_TAG 7040ac095c0d227b4cf6431ea224ae4938ec4ef1
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(astra)

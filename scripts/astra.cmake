include(FetchContent)

message("Fetching Astra UI")
FetchContent_Declare(
    astra
    GIT_REPOSITORY https://github.com/skript023/astra-ui.git
    GIT_TAG 27caaeed7eec139b67078ffcedec357a8697410a
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(astra)

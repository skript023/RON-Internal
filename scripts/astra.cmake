include(FetchContent)

message("Fetching Astra UI")
FetchContent_Declare(
    astra
    GIT_REPOSITORY https://github.com/skript023/astra-ui.git
    GIT_TAG c8cb815315805e35e1beac7fae050ce6acb088c7
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(astra)

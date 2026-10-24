include(FetchContent)

message("Fetching Astra UI")
FetchContent_Declare(
    astra
    GIT_REPOSITORY https://github.com/skript023/astra-ui.git
    GIT_TAG 8f1c551f6e50738043f93d079e1cbd974b7ac216
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(astra)

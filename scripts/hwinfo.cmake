include(FetchContent)

FetchContent_Declare(
    hwinfo
    GIT_REPOSITORY https://github.com/lfreist/hwinfo.git
    GIT_TAG        894ed646a85ae2c52c04c9426ba778dac7503087
    GIT_PROGRESS TRUE
) 
message("hwinfo")

FetchContent_MakeAvailable(hwinfo)
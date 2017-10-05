add_library(irohad-flags
    flags.cpp
    ../flags.hpp
    )
target_link_libraries(irohad-flags
    gflags
    logger
    )

add_library(irohad-config
    gflags_config.hpp
    gflags_config.cpp
    ../config.hpp
    )
target_link_libraries(irohad-config
    irohad-flags
    )

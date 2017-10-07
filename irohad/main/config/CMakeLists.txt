add_library(irohad-flags
    impl/flags.cpp
    flags.hpp
    )
target_link_libraries(irohad-flags
    gflags
    logger
    )

add_library(irohad-config
    impl/gflags_config.hpp
    impl/gflags_config.cpp
    config.hpp
    )
target_link_libraries(irohad-config
    irohad-flags
    )

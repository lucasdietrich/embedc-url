if(CONFIG_EMBEDC_URL_PARSER)
    zephyr_library()

    set(lib ${ZEPHYR_CURRENT_MODULE_DIR})

    zephyr_include_directories(${lib}/include)
        
    FILE(GLOB_RECURSE sources ${lib}/src/*.c*)
    zephyr_library_sources(${sources})
endif()
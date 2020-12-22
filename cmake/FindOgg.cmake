include(FindPackageHandleStandardArgs)

find_path(Ogg_INCLUDE_DIR ogg/ogg.h
          PATHS /usr/local/include /usr/include
)

find_library(Ogg_LIBRARY
            NAMES ogg libogg libogg_static
            PATHS /usr/local/lib /usr/lib
)

find_package_handle_standard_args(Ogg REQUIRED_VARS Ogg_INCLUDE_DIR Ogg_LIBRARY)

if(NOT TARGET Ogg::ogg)
    add_library(Ogg::ogg UNKNOWN IMPORTED)
    set_target_properties(
        Ogg::ogg PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${Ogg_INCLUDE_DIR}
        IMPORTED_LOCATION ${Ogg_LIBRARY}
    )
endif()

set(Ogg_INCLUDE_DIRS ${Ogg_INCLUDE_DIR})
set(Ogg_LIBRARIES ${Ogg_LIBRARY})

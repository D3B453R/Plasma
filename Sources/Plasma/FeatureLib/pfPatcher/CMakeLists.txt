set(pfPatcher_SOURCES
    plManifests.cpp
    pfPatcher.cpp
)

set(pfPatcher_HEADERS
    plManifests.h
    pfPatcher.h
)

add_library(pfPatcher STATIC ${pfPatcher_SOURCES} ${pfPatcher_HEADERS})

target_link_libraries(pfPatcher
    PUBLIC
        CoreLib
        pnEncryption
)

source_group("Source Files" FILES ${pfPatcher_SOURCES})
source_group("Header Files" FILES ${pfPatcher_HEADERS})

set(plPhysX_SOURCES
    plGenericPhysical.cpp
    plLOSDispatch.cpp
    plPXConvert.cpp
    plPXCooking.cpp
    plPXLOSDispatch.cpp
    plPXPhysical.cpp
    plPXPhysicalControllerCore.cpp
    plPXSimulation.cpp
    plPXSubWorld.cpp
    plSimulationMgr.cpp
)

set(plPhysX_HEADERS
    plLOSDispatch.h
    plPhysXAPI.h
    plPhysXCreatable.h
    plPXConvert.h
    plPXCooking.h
    plPXPhysical.h
    plPXPhysicalControllerCore.h
    plPXSimDefs.h
    plPXSimulation.h
    plPXSubWorld.h
    plSimulationMgr.h
)

add_library(plPhysX STATIC ${plPhysX_SOURCES} ${plPhysX_HEADERS})

target_link_libraries(plPhysX
    PUBLIC
        CoreLib
        PhysX::PhysX
        plPhysical
)

source_group("Source Files" FILES ${plPhysX_SOURCES})
source_group("Header Files" FILES ${plPhysX_HEADERS})

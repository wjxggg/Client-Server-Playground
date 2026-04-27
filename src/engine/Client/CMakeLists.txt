# Target files
set(SHARED_DIR ${CMAKE_SOURCE_DIR}/src/engine/Shared)
file(GLOB_RECURSE SHARED_FILES ${SHARED_DIR}/*.c ${SHARED_DIR}/*.cpp ${SHARED_DIR}/*.h ${SHARED_DIR}/*.hpp ${SHARED_DIR}/*.ipp ${SHARED_DIR}/*.tpp)
file(GLOB_RECURSE SOURCE_FILES *.c *.cpp *.h *.hpp *.ipp *.tpp)

set(CURRENT_TARGET Client)

add_executable(${CURRENT_TARGET}
	${SHARED_FILES}
	${SOURCE_FILES}
)

target_include_directories(${CURRENT_TARGET} PRIVATE
	${INCLUDE_DIR}
	${INCLUDE_DIR}/${CURRENT_TARGET}
	${INCLUDE_DIR}/Shared
	${SHARED_DIR}
	./
)

target_include_directories(${CURRENT_TARGET} SYSTEM PRIVATE
	${THIRD_PARTY_DIR}
)

# Target options
target_compile_definitions(${CURRENT_TARGET} PRIVATE BE_HOST_CLIENT)

set_target_properties(${CURRENT_TARGET} PROPERTIES ENABLE_EXPORTS ON)

# Installation
install(TARGETS ${CURRENT_TARGET}
	RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
)

# Platform libraries
if (WIN32)
	target_link_libraries(${CURRENT_TARGET} PRIVATE
		winmm.lib
	)
endif ()

# Third-party libraries
find_package(SDL3 REQUIRED CONFIG PATHS ${SDL_INSTALL_DIR}/lib/cmake/SDL3 NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
find_package(spdlog REQUIRED CONFIG PATHS ${SPDLOG_INSTALL_DIR}/lib/cmake/spdlog NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)

target_link_libraries(${CURRENT_TARGET} PRIVATE
	SDL3::SDL3-static
	spdlog::spdlog
)

target_compile_definitions(${CURRENT_TARGET} PRIVATE
	RAPIDJSON_HAS_STDSTRING=1
)

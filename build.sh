SCRIPT_NAME="build"
ROOT_DIR=$(cd $(dirname "$0"); pwd)

case "$OSTYPE" in
    cygwin*|msys*|mingw*)
        ROOT_DIR=$(cygpath -m "${ROOT_DIR}")
esac

mkdir -p ./log
exec > >(tee ./log/${SCRIPT_NAME}.log) 2>&1

echo "${SCRIPT_NAME}.sh start"
echo

TARGET_TRIPLE=$1
TOOLCHAIN_FILE="${ROOT_DIR}/toolchain/zig-toolchain.cmake"

mkdir -p ./build/${TARGET_TRIPLE} ./game/${TARGET_TRIPLE} ./lib/${TARGET_TRIPLE}

SDL_BUILD_DIR="${ROOT_DIR}/build/${TARGET_TRIPLE}/SDL"
SDL_INSTALL_DIR="${ROOT_DIR}/lib/${TARGET_TRIPLE}/SDL"

if [ ! -d ${SDL_INSTALL_DIR} ];then
	echo
	echo "Building SDL..."
	echo

	SDL_OPTIONS=(
		"-DCMAKE_BUILD_TYPE=Release"
		"-DCMAKE_CXX_SCAN_FOR_MODULES=OFF"
		"-DCMAKE_INSTALL_MESSAGE=LAZY"
		"-DCMAKE_INSTALL_PREFIX=${SDL_INSTALL_DIR}"
		"-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}"
		"-DSDL_DEPS_SHARED=OFF"
		"-DSDL_EXAMPLES=OFF"
		"-DSDL_INSTALL_DOCS=OFF"
		"-DSDL_SHARED=OFF"
		"-DSDL_STATIC=ON"
		"-DSDL_TEST_LIBRARY=OFF"
		"-DZIG_TARGET=${TARGET_TRIPLE}"
	)

	cmake -G "Ninja" -S ./src/third-party/SDL -B ${SDL_BUILD_DIR} ${SDL_OPTIONS[@]}
	cmake --build ${SDL_BUILD_DIR} --config Release
	cmake --install ${SDL_BUILD_DIR} --config Release

	echo
	echo "Finished building SDL"
	echo
fi

SPDLOG_BUILD_DIR="${ROOT_DIR}/build/${TARGET_TRIPLE}/spdlog"
SPDLOG_INSTALL_DIR="${ROOT_DIR}/lib/${TARGET_TRIPLE}/spdlog"

if [ ! -d ${SPDLOG_INSTALL_DIR} ];then
	echo
	echo "Building spdlog..."
	echo

	SDPLOG_OPTIONS=(
		"-DCMAKE_BUILD_TYPE=Release"
		"-DCMAKE_CXX_SCAN_FOR_MODULES=OFF"
		"-DCMAKE_INSTALL_MESSAGE=LAZY"
		"-DCMAKE_INSTALL_PREFIX=${SPDLOG_INSTALL_DIR}"
		"-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}"
		"-DSPDLOG_BUILD_EXAMPLE=OFF"
		"-DSPDLOG_DISABLE_DEFAULT_LOGGER=ON"
		"-DSPDLOG_NO_EXCEPTIONS=ON"
		"-DSPDLOG_NO_THREAD_ID=ON"
		"-DSPDLOG_SYSTEM_INCLUDES=ON"
		"-DSPDLOG_USE_STD_FORMAT=ON"
		"-DZIG_TARGET=${TARGET_TRIPLE}"
	)

	cmake -G "Ninja" -S ./src/third-party/spdlog -B ${SPDLOG_BUILD_DIR} ${SDPLOG_OPTIONS[@]}
	cmake --build ${SPDLOG_BUILD_DIR} --config Release
	cmake --install ${SPDLOG_BUILD_DIR} --config Release

	echo
	echo "Finished building spdlog"
	echo
fi

GAME_BUILD_DIR="${ROOT_DIR}/build/${TARGET_TRIPLE}/game"
GAME_INSTALL_DIR="${ROOT_DIR}/game/${TARGET_TRIPLE}/game"

GAME_OPTIONS=(
	"-DCMAKE_BUILD_TYPE=Debug"
	"-DCMAKE_CXX_SCAN_FOR_MODULES=OFF"
	"-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
	"-DCMAKE_INSTALL_MESSAGE=LAZY"
	"-DCMAKE_INSTALL_PREFIX=${GAME_INSTALL_DIR}"
	"-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}"
	"-DSDL_INSTALL_DIR=${SDL_INSTALL_DIR}"
	"-DSPDLOG_INSTALL_DIR=${SPDLOG_INSTALL_DIR}"
	"-DZIG_TARGET=${TARGET_TRIPLE}"
)

cmake -G "Ninja" -S ./ -B ${GAME_BUILD_DIR} ${GAME_OPTIONS[@]}
cmake --build ${GAME_BUILD_DIR} --config Debug
cmake --install ${GAME_BUILD_DIR} --config Debug

echo
echo "${SCRIPT_NAME}.sh finished"
echo
########################################################################################################################
# CMake build script for Qualcomm BREW: 27-Jun-2024
#
cmake_minimum_required(VERSION 3.16)

########################################################################################################################
# Project section: Benchmark
#
project(2048-BREW LANGUAGES C ASM VERSION "1.0.0")

include_directories(
	${PROJECT_SOURCE_DIR}
)

add_definitions("-DLINUX_BOGOMIPS")
#add_definitions("-DNO_ASM")

add_brew_module(brew_2048
	${BREW_PLATFORM_SDK_ROOT}/src/AEEModGen.c
	${BREW_PLATFORM_SDK_ROOT}/src/AEEAppGen.c

	brew_benchmark.c
	delay_armv4t_GCC.S
	dhry_1.c
	dhry_2.c
	phases.c
)

########################################################################################################################
# CMake build script for Qualcomm BREW: 11-Jul-2024
#
cmake_minimum_required(VERSION 3.16)

########################################################################################################################
# Project section: FireEffect
#
project(FireEffect LANGUAGES C VERSION "1.0.0")

include_directories(
	${PROJECT_SOURCE_DIR}
)

add_brew_module(brew_fire_effect
	${BREW_PLATFORM_SDK_ROOT}/src/AEEModGen.c
	${BREW_PLATFORM_SDK_ROOT}/src/AEEAppGen.c

	brew_fire_effect.c
)

#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  taken from github.com/osmcode/osm-gis-export
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


#-----------------------------------------------------------------------------
#
#  Project version
#
#-----------------------------------------------------------------------------

project(Cerepso-Postgres-Backend)

set(AUTHOR "Michael Reichert <nakaner@gmx.net>")


#-----------------------------------------------------------------------------
#
#  Documentation
#
#-----------------------------------------------------------------------------

add_subdirectory(doc)

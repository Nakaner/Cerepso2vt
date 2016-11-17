#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

add_executable(vectortile-generator vectortile-generator.cpp vector_tile.cpp mytable.cpp bounding_box.cpp)
target_link_libraries(vectortile-generator ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES} ${GEOS_LIBRARY} ${PostgreSQL_LIBRARY})
install(TARGETS vectortile-generator DESTINATION bin)


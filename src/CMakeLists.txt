#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

add_executable(vectortile-generator vectortile-generator.cpp osmvectortileimpl.cpp osm_data_table.cpp bounding_box.cpp jobs_database.cpp)
target_link_libraries(vectortile-generator ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES} ${GEOS_LIBRARY} ${PostgreSQL_LIBRARY})
install(TARGETS vectortile-generator DESTINATION bin)


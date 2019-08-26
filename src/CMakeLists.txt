#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

add_executable(vectortile-generator vectortile-generator.cpp cerepso_data_access.cpp osm_data_table.cpp bounding_box.cpp jobs_database.cpp)
target_link_libraries(vectortile-generator ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES} ${GEOS_LIBRARY} ${PostgreSQL_LIBRARY})
install(TARGETS vectortile-generator DESTINATION bin)


#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

add_executable(vectortile-generator vectortile-generator.cpp input/cerepso_data_access.cpp input/osm2pgsql_data_access.cpp osm_data_table.cpp bounding_box.cpp jobs_database.cpp input/nodes_provider.cpp input/nodes_db_provider.cpp input/nodes_flatnode_provider.cpp input/nodes_provider_factory.cpp input/metadata_fields.cpp input/column_config_parser.cpp)
target_link_libraries(vectortile-generator ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES} ${GEOS_LIBRARY} ${PostgreSQL_LIBRARY})
install(TARGETS vectortile-generator DESTINATION bin)


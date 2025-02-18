add_subdirectory(1.0)
add_subdirectory(1.2)
add_subdirectory(1.3)
add_subdirectory(1.4)
add_subdirectory(1.5)
add_subdirectory(1.6)
add_subdirectory(1.7)
add_subdirectory(1.8)
add_subdirectory(1.9)

add_custom_target(schema)
add_dependencies(schema schema1_9)

# Generate the EmbeddedSdf.cc file, which contains all the supported SDF
# descriptions in a map of strings. The parser.cc file uses EmbeddedSdf.hh.
execute_process(
  COMMAND ${RUBY} ${CMAKE_SOURCE_DIR}/sdf/embedSdf.rb
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/sdf"
  OUTPUT_FILE "${PROJECT_BINARY_DIR}/src/EmbeddedSdf.cc"
)

# Generate aggregated SDF description files for use by the sdformat.org 
# website. If the description files change, the generated full*.sdf files need 
# to be removed before running this target.
if (IGN_PROGRAM)

  # Update this list as new sdformat spec versions are added.
  set(sdf_desc_versions 1.4 1.5 1.6 1.7 1.8 1.9)

  set(description_targets)
  foreach(desc_ver ${sdf_desc_versions})
    string(REPLACE "." "-" desc_ver_dash ${desc_ver})
    list(APPEND description_targets ${CMAKE_CURRENT_BINARY_DIR}/full_${desc_ver_dash}.sdf)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/full_${desc_ver_dash}.sdf
      COMMAND
        ${CMAKE_COMMAND} -E env IGN_CONFIG_PATH=${CMAKE_BINARY_DIR}/test/conf
        ${IGN_PROGRAM}
      ARGS sdf -d ${desc_ver} > ${CMAKE_CURRENT_BINARY_DIR}/full_${desc_ver_dash}.sdf
      COMMENT "Generating full description for spec ${desc_ver}"
      VERBATIM)
  endforeach()
  add_custom_target(sdf_descriptions DEPENDS ${description_targets} ${PROJECT_LIBRARY_TARGET_NAME})
endif()

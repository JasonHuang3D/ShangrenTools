
set(jutils_target_name "JUtils")

file(GLOB_RECURSE jutils_source 
    "*.h" "*.cpp" "*.hpp"
)

# Add to source group, TREE will enable cmake to config the folder layout automatically 
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${jutils_source})

# Create the static library
add_library(${jutils_target_name} STATIC ${jutils_source})

# Config common include and link
target_include_directories(${jutils_target_name} 
PUBLIC
    ${frameworks_dir}
)

# Config link dependencies
target_link_libraries(${jutils_target_name} 
PRIVATE
    build_settings
    vorbrodt_blog
)

# Call the helper function
config_target_properties(${jutils_target_name})

set_target_properties(${jutils_target_name} PROPERTIES
    FOLDER ${frameworks_dir_name}
)
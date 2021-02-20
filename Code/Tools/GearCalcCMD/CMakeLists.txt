# Set App target name
set(app_target_name GearCalcCMD)

# set the source files to compile
file(GLOB_RECURSE file_source "Src/*.h" "Src/*.cpp" "Src/*.hpp"

)
# Add to source group, TREE will enable cmake to config the folder layout automatically 
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${file_source})

# Set asset files
file(GLOB file_assets 
)
# Add to source group, TREE will enable cmake to config the folder layout automatically 
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${file_assets})

# Set resource files
set(file_resources
)

# Add to source group, TREE will enable cmake to config the folder layout automatically 
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${file_resources})

# create the executable
add_app(${app_target_name} ${tools_dir_name} false "${file_source}" "${file_assets}" "${file_resources}")

# Config includes
target_include_directories(${app_target_name}
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Src
)

# Config links
target_link_libraries(${app_target_name} 
PRIVATE
    build_settings
    JUtils
    vorbrodt_blog
    nlohmann_json
)

# Add target name
target_compile_definitions(${app_target_name}
PRIVATE
    APP_NAME="${app_target_name}"
)
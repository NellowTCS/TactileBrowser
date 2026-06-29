# Compile core + lexbor sources directly into this component instead of
# linking separate static libraries as this avoids elf_loader stripping.
file(GLOB_RECURSE CORE_SOURCES ../../tactilebrowser_core/src/*.cpp)
file(GLOB_RECURSE LEXBOR_SOURCES
    ../../tactilebrowser_core/lexbor/source/lexbor/core/*.c
    ../../tactilebrowser_core/lexbor/source/lexbor/css/*.c
    ../../tactilebrowser_core/lexbor/source/lexbor/css/syntax/*.c
    ../../tactilebrowser_core/lexbor/source/lexbor/dom/*.c
    ../../tactilebrowser_core/lexbor/source/lexbor/encoding/*.c
    ../../tactilebrowser_core/lexbor/source/lexbor/engine/*.c
    ../../tactilebrowser_core/lexbor/source/lexbor/html/*.c
    ../../tactilebrowser_core/lexbor/source/lexbor/ns/*.c
    ../../tactilebrowser_core/lexbor/source/lexbor/punycode/*.c
    ../../tactilebrowser_core/lexbor/source/lexbor/selectors/*.c
    ../../tactilebrowser_core/lexbor/source/lexbor/style/*.c
    ../../tactilebrowser_core/lexbor/source/lexbor/tag/*.c
    ../../tactilebrowser_core/lexbor/source/lexbor/unicode/*.c
    ../../tactilebrowser_core/lexbor/source/lexbor/url/*.c
    ../../tactilebrowser_core/lexbor/source/lexbor/utils/*.c
)
file(GLOB LEXBOR_POSIX_SOURCES
    ../../tactilebrowser_core/lexbor/source/lexbor/ports/posix/lexbor/core/*.c
)
list(FILTER LEXBOR_POSIX_SOURCES EXCLUDE REGEX ".*fs\\.c$")
list(APPEND LEXBOR_SOURCES ${LEXBOR_POSIX_SOURCES})
file(GLOB_RECURSE SOURCE_FILES Source/*.cpp *.cpp)

# Register component
idf_component_register(
    SRCS ${SOURCE_FILES} ${CORE_SOURCES} ${LEXBOR_SOURCES}
    # Include main sources + core public headers + lexbor sources so
    # includes like <lexbor/html/html.h> are found when building the app.
    INCLUDE_DIRS
      "Source"
      "../../tactilebrowser_core/include"
      "../../tactilebrowser_core/lexbor/source"
    REQUIRES TactilitySDK esp_http_client newlib
)

# Force C standard
set_target_properties(${COMPONENT_LIB} PROPERTIES C_STANDARD 99)
target_compile_definitions(${COMPONENT_LIB} PRIVATE LEXBOR_STATIC)

target_link_libraries(${COMPONENT_LIB} PRIVATE
    esp_http_client newlib m TactilityC lvgl
)

# Suppress warnings from #include_next in Lexbor
add_compile_options(-Wno-include-next)

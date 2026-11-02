# Compile core + lexbor sources directly into this component instead of
# linking separate static libraries as this avoids elf_loader stripping.
file(GLOB_RECURSE CORE_SOURCES ../../fdm/src/*.cpp)
file(GLOB_RECURSE LEXBOR_SOURCES
    ../../fdm/lexbor/source/lexbor/core/*.c
    ../../fdm/lexbor/source/lexbor/css/*.c
    ../../fdm/lexbor/source/lexbor/css/syntax/*.c
    ../../fdm/lexbor/source/lexbor/dom/*.c
    ../../fdm/lexbor/source/lexbor/encoding/*.c
    ../../fdm/lexbor/source/lexbor/engine/*.c
    ../../fdm/lexbor/source/lexbor/html/*.c
    ../../fdm/lexbor/source/lexbor/ns/*.c
    ../../fdm/lexbor/source/lexbor/punycode/*.c
    ../../fdm/lexbor/source/lexbor/selectors/*.c
    ../../fdm/lexbor/source/lexbor/style/*.c
    ../../fdm/lexbor/source/lexbor/tag/*.c
    ../../fdm/lexbor/source/lexbor/unicode/*.c
    ../../fdm/lexbor/source/lexbor/url/*.c
    ../../fdm/lexbor/source/lexbor/utils/*.c
)
file(GLOB LEXBOR_POSIX_SOURCES
    ../../fdm/lexbor/source/lexbor/ports/posix/lexbor/core/*.c
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
      "../../fdm/include"
      "../../fdm/lexbor/source"
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

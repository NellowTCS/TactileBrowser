# This file will be configured to contain variables for CPack. These variables
# should be set in the CMake list file of the project before CPack module is
# included. The list of available CPACK_xxx variables and their associated
# documentation may be obtained using
#  cpack --help-variable-list
#
# Some variables are common to all generators (e.g. CPACK_PACKAGE_NAME)
# and some are specific to a generator
# (e.g. CPACK_NSIS_EXTRA_INSTALL_COMMANDS). The generator specific variables
# usually begin with CPACK_<GENNAME>_xxxx.


set(CPACK_BUILD_SOURCE_DIRS "/Users/vaishnavi/Documents/Neel/Stuff/TactileBrowser/src;/Users/vaishnavi/Documents/Neel/Stuff/TactileBrowser/src/build-esp32s3")
set(CPACK_CMAKE_GENERATOR "Ninja")
set(CPACK_COMPONENT_UNSPECIFIED_HIDDEN "TRUE")
set(CPACK_COMPONENT_UNSPECIFIED_REQUIRED "TRUE")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_FILE "/opt/homebrew/share/cmake/Templates/CPack.GenericDescription.txt")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_SUMMARY "TactileBrowser built using CMake")
set(CPACK_GENERATOR "TGZ")
set(CPACK_INNOSETUP_ARCHITECTURE "x86")
set(CPACK_INSTALL_CMAKE_PROJECTS "/Users/vaishnavi/Documents/Neel/Stuff/TactileBrowser/src/build-esp32s3;TactileBrowser;ALL;/")
set(CPACK_INSTALL_PREFIX "/usr/local")
set(CPACK_MODULE_PATH "/Users/vaishnavi/esp/esp-idf/tools/cmake;/Users/vaishnavi/esp/esp-idf/tools/cmake/third_party")
set(CPACK_NSIS_DISPLAY_NAME "lexbor 2.5.0")
set(CPACK_NSIS_INSTALLER_ICON_CODE "")
set(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
set(CPACK_NSIS_PACKAGE_NAME "lexbor 2.5.0")
set(CPACK_NSIS_UNINSTALL_NAME "Uninstall")
set(CPACK_OBJCOPY_EXECUTABLE "/Users/vaishnavi/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20241119/xtensa-esp-elf/bin/xtensa-esp32-elf-objcopy")
set(CPACK_OBJDUMP_EXECUTABLE "/Users/vaishnavi/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20241119/xtensa-esp-elf/bin/xtensa-esp32-elf-objdump")
set(CPACK_OUTPUT_CONFIG_FILE "/Users/vaishnavi/Documents/Neel/Stuff/TactileBrowser/src/build-esp32s3/CPackConfig.cmake")
set(CPACK_PACKAGE_CONTACT "Alexander Borisov")
set(CPACK_PACKAGE_DEFAULT_LOCATION "/")
set(CPACK_PACKAGE_DESCRIPTION_FILE "/opt/homebrew/share/cmake/Templates/CPack.GenericDescription.txt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HTML Renderer")
set(CPACK_PACKAGE_FILE_NAME "lexbor-2.5.0-1.x86_64")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "lexbor 2.5.0")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "lexbor 2.5.0")
set(CPACK_PACKAGE_NAME "lexbor")
set(CPACK_PACKAGE_RELEASE "1")
set(CPACK_PACKAGE_RELOCATABLE "true")
set(CPACK_PACKAGE_VENDOR "Alexander Borisov")
set(CPACK_PACKAGE_VERSION "2.5.0")
set(CPACK_PACKAGE_VERSION_MAJOR "3")
set(CPACK_PACKAGE_VERSION_MINOR "6")
set(CPACK_PACKAGE_VERSION_PATCH "2")
set(CPACK_READELF_EXECUTABLE "/Users/vaishnavi/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20241119/xtensa-esp-elf/bin/xtensa-esp32-elf-readelf")
set(CPACK_RESOURCE_FILE_LICENSE "/opt/homebrew/share/cmake/Templates/CPack.GenericLicense.txt")
set(CPACK_RESOURCE_FILE_README "/opt/homebrew/share/cmake/Templates/CPack.GenericDescription.txt")
set(CPACK_RESOURCE_FILE_WELCOME "/opt/homebrew/share/cmake/Templates/CPack.GenericWelcome.txt")
set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
set(CPACK_SET_DESTDIR "OFF")
set(CPACK_SOURCE_7Z "ON")
set(CPACK_SOURCE_GENERATOR "7Z;ZIP")
set(CPACK_SOURCE_OUTPUT_CONFIG_FILE "/Users/vaishnavi/Documents/Neel/Stuff/TactileBrowser/src/build-esp32s3/CPackSourceConfig.cmake")
set(CPACK_SOURCE_ZIP "ON")
set(CPACK_SYSTEM_NAME "Generic")
set(CPACK_THREADS "1")
set(CPACK_TOPLEVEL_TAG "Generic")
set(CPACK_WIX_SIZEOF_VOID_P "4")

if(NOT CPACK_PROPERTIES_FILE)
  set(CPACK_PROPERTIES_FILE "/Users/vaishnavi/Documents/Neel/Stuff/TactileBrowser/src/build-esp32s3/CPackProperties.cmake")
endif()

if(EXISTS ${CPACK_PROPERTIES_FILE})
  include(${CPACK_PROPERTIES_FILE})
endif()

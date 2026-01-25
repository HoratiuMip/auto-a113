# Copy and configure the following in your cmake script before adding the auto-a113 subdirectory.
# Due to thinking about her I completely forgot what BAS stands for two minutes after making the file.

# Location of auto-a113.
set( A113_ROOT_DIR "" )

# Build target plate.
# - "uC" - target is a microcontroller device.
# - "OS" - target is an operating system ready device. 
set( A113_TARGET_PLATE "" )

# Target operating system. This is relevant and needs to be set only if A113_TARGET_PLATE was configured to "OS".
# - "Windows"
set( A113_TARGET_OS "" ) 

# Add auto-a113.
add_subdirectory( "${A113_ROOT_DIR}" "#{CMAKE_CURRENT_BINARY_DIR}/auto-a113" )

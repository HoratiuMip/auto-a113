# Copy and configure the following in your cmake script before adding the auto-a113 subdirectory.
# Due to thinking about her I completely forgot what BAS stands for two minutes after making the file.

# VVV COPY FROM HERE VVV

# ===== auto-a113 BAS begin =====
# Location of auto-a113.
set( A113_ROOT_DIR "" )

# Build target plate.
# - "uC" - target is a microcontroller device.
# - "OS" - target is an operating system ready device. 
set( A113_TARGET_PLATE "" )

# Target operating system.
# - OSp:
#   - "Windows"
# - uCp:
#   - <empty>
#   - "FreeRTOS"
set( A113_TARGET_OS "" ) 

# Target platform.
# - OSp:
#   - <empty>
# - uCp:
#   - "esp32"
set( A113_TARGET_PLATFORM "" )

# Clockworks to configure and build.
#  - "immersive" - The Immersive framework, used for GUI and 3D rendering.
set( A113_MAKE_CLOCKWORKS 
    "" 
)

# * Which external components to make, indicated by a string containing 'y' and 'n' ( for "yes" and "no" ), in the order in which they are built in the script.
# Example: "ynnny" - makes the first and fifth external component.
set( A113_EXCOM_POPCOUNT
    "y"
)

# Add auto-a113.
add_subdirectory( "${A113_ROOT_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/auto-a113" )
# ===== auto-a113 BAS end =====

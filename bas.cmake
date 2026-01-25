# Due to thinking about her I completely forgot what BAS stands for two minutes after making the file.

# Copy and configure the following in tour cmake script before adding the auto-a113 subdirectory.
# Add -DA113_USE_BAS as an argument to your cmake command line to use this configuration file.

# Build target plate.
# - "uC" - target is a microcontroller device.
# - "OS" - target is an operating system ready device. 
set( A113_TARGET_PLATE "" )

# Target operating system. This is relevant and needs to be set only if A113_TARGET_PLATE was configured to "OS".
# - "Windows"
set( A113_TARGET_OS "" ) 

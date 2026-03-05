add_library(adi INTERFACE)

# Collect C/C++ source files in this directory (recursive).
# Note: provide each glob pattern as a separate argument; combining
# patterns into a single string is incorrect and doesn't match files.
file(GLOB_RECURSE SRC_FILES
	"${CMAKE_CURRENT_LIST_DIR}/*.c"
)
file(GLOB_RECURSE HEAD_FILES
	"${CMAKE_CURRENT_LIST_DIR}/*.h"
)

# Add sources/headers to the INTERFACE target only when files exist.
if(SRC_FILES)
	target_sources(adi INTERFACE ${SRC_FILES})
endif()

if(HEAD_FILES)
	# Adding headers to target_sources is optional, but can help IDEs.
	target_sources(adi INTERFACE ${HEAD_FILES})
endif()

target_include_directories(adi INTERFACE ${CMAKE_CURRENT_LIST_DIR})
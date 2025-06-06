set(CMAKE_AR gcc-ar-11${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_ASM_COMPILER gcc-11${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_C_COMPILER gcc-11${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_CXX_COMPILER g++-11${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_LINKER ld${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_OBJCOPY objcopy${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")
set(CMAKE_RANLIB ranlib${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")
set(CMAKE_SIZE size${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")
set(CMAKE_STRIP strip${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")
set(CMAKE_GCOV gcov-11${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")

set(CMAKE_C_FLAGS "${APP_C_FLAGS} -Wall  " CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS "${APP_CXX_FLAGS} ${CMAKE_C_FLAGS}" CACHE INTERNAL "")

set(CMAKE_C_FLAGS_DEBUG "-g" CACHE INTERNAL "")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" CACHE INTERNAL "")

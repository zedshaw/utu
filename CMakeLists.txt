project(Utu)

set(CMAKE_C_FLAGS "-g -Wall -W -Wpointer-arith -Wno-unused-parameter -Wno-unused-function -Wunused-variable -Wunused-value -Werror")
#set(CMAKE_C_FLAGS "-O2 -DNDEBUG -Wall -W -Wpointer-arith")

subdirs(src tests)
include(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
include(${CMAKE_ROOT}/Modules/CheckLibraryExists.cmake)
find_program(DOXYGEN doxygen)
find_program(RAKE rake)

check_include_file(tfm.h HAS_TFM)
check_include_file(tommath.h HAS_LTM)
check_include_file(tomcrypt.h HAS_LTC)
check_include_file(myriad/myriad.h HAS_MYRIAD)

IF(HAS_TFM)
ADD_DEFINITIONS(-DHAS_TFM -DTFM_DESC)
set(MATH_LIB tfm)
ENDIF(HAS_TFM)

IF(HAS_LTM)
ADD_DEFINITIONS(-DHAS_LTM -DLTM_DESC)
set(MATH_LIB tommath)
ENDIF(HAS_LTM)

MESSAGE("Building with mathlib ${MATH_LIB}")

add_custom_target(docs ${DOXYGEN} doc/Doxyfile)
add_custom_target(rubyclient ${RAKE} WORKING_DIRECTORY src/lang/ruby/utuclient)

enable_testing()
add_test(testutu tests/testrunner)


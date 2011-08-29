
IF(HAS_MYRIAD)
add_executable(utuserver server.c)

target_link_libraries(utuserver utu tomcrypt ${MATH_LIB} myriad m)

install(TARGETS utuserver RUNTIME DESTINATION bin)
ENDIF(HAS_MYRIAD)

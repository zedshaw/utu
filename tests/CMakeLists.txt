include_directories(../src)

IF(HAS_MYRIAD)
  set(testsource
    test_frame.c
    test_hub.c test_member.c
    test_message.c 
    test_heap.c
    test_queue.c test_routing.c
    test_stackish.c 
    test_crypto.c 
    test_peer.c
    )
  set(testlibs utu tomcrypt ${MATH_LIB} myriad sqlite3 pthread m gcov)

  add_executable(testrunner testrunner.c ${testsource})

  target_link_libraries(testrunner ${testlibs})

  add_executable(cutgen cutgen.c)

  add_custom_command(OUTPUT testrunner.c COMMAND ./cutgen -o testrunner.c ${testsource})
  add_dependencies(testrunner cutgen)
ENDIF(HAS_MYRIAD)


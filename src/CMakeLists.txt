subdirs(server client)

include_directories(.)

add_custom_command(OUTPUT stackish/stackish.c COMMAND ragel stackish/stackish.rl | rlgen-cd -o stackish/stackish.c)
add_custom_command(OUTPUT hub/hub.c COMMAND ragel hub/hub.rl | rlgen-cd -o hub/hub.c)
add_custom_command(OUTPUT hub/connection.c COMMAND ragel hub/connection.rl | rlgen-cd -o hub/connection.c)

IF(HAS_MYRIAD)
  add_library(utu
    stackish/node.c stackish/stackish.c 
    protocol/peer.c protocol/frame.c protocol/crypto.c protocol/message.c
    hub/member.c hub/queue.c hub/connection.c hub/hub.c
    hub/connection_state.c hub/hub_state.c hub/routing.c hub/commands.c
    hub/heap.c hub/info.c
    )

  install(TARGETS utu
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

  install(FILES
    hub/hub.h hub/member.h hub/heap.h
    hub/queue.h hub/routing.h
    DESTINATION include/utu/hub )

  install(FILES
    protocol/crypto.h protocol/frame.h
    protocol/message.h protocol/peer.h
    DESTINATION include/utu/protocol )

  install(FILES
    stackish/node.h stackish/ragel.h
    stackish/ragel_declare.h stackish/stackish.h
    DESTINATION include/utu/stackish )
ELSE(HAS_MYRIAD)
  MESSAGE("NO myriad found, you'll only get utumendicant no utuserver.")
ENDIF(HAS_MYRIAD)

add_executable(utumendicant 
  client/client.c client/io.c client/proxy.c
  stackish/node.c stackish/stackish.c 
  protocol/crypto.c myriad/bstring/bstrlib.c)

install(TARGETS utumendicant RUNTIME DESTINATION bin)

target_link_libraries(utumendicant tomcrypt ${MATH_LIB})

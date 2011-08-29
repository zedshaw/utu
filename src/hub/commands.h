#ifndef utu_hub_commands_h
#define utu_hub_commands_h

#include "hub/hub.h"

/** 
 * @function Hub_commands_register
 * @brief Registers all the available commands with the hub.
 * @param hub : Hub to register with.
 * @return int : Whether it worked.
 */
int Hub_commands_register(Hub *hub);

/** 
 * @function Hub_service_message
 * @brief Handles routing or servicing a message from a user.
 * @param state : The connection to process a message from.
 * @return int : Whether that was a valid message.
 */
int Hub_service_message(struct ConnectionState *state);


#endif

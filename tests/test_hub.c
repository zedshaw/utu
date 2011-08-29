#include <stdio.h>
#include <stdlib.h>
#include "cut.h"
#include "hub/hub.h"


int conn_exec(ConnectionState *fsm, const char *events, int initialize, int finalize)
{
  if(initialize) ASSERT(ConnectionState_init(fsm), "failed to init ConnectionState");

  while(*events) {
    if(ConnectionState_exec(fsm, *events) == -1) return 0;
    events++;
  }

  if(finalize) ASSERT_EQUALS(ConnectionState_finish(fsm), 1, "failed to finish machine");

  return 1;
}


int hub_exec(Hub *fsm, const char *events)
{
  while(*events) {
    if(Hub_exec(fsm, *events) == -1) return 0;
    events++;
  }

  return 1;
}

void __CUT_BRINGUP__HubTest( void ) 
{
}


void __CUT__Hub_Hub_state()
{
  Hub *hub = NULL;

  const char simple_events[] = {0};

  Hub_init("testrunner");

  hub = Hub_create(bfromcstr("0.0.0.0"), bfromcstr("4000"), bfromcstr("utu"), NULL);
  ASSERT(hub != NULL, "failed to create hub");

  hub_exec(hub, simple_events);

  ASSERT(Hub_destroy(hub) == 1, "failed on hub destroy");
}


void __CUT_TAKEDOWN__HubTest( void ) 
{
}


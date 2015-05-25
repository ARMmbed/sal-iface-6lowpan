/*
 * Copyright (c) 2015 ARM. All rights reserved.
 */
#ifndef _NODE_TASKLET_MAIN_
#define _NODE_TASKLET_MAIN_
#include "ns_types.h"
#include "eventOS_event.h"

/*
 * \brief Network interface ID.
 */
extern int8_t network_interface_id;

/*
 * \brief Main tasklet for handling NanoStack events
 * \param event
 */
extern void tasklet_main(arm_event_s *event);

/*
 * \brief Register callback for network ready indication
 * \param function to be called when network is established
 */
extern void node_tasklet_register_network_ready_cb(void (*network_ready_cb)(void));

#endif /* _NODE_TASKLET_MAIN_ */

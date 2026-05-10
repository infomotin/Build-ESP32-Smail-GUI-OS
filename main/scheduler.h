/*
 * Scheduler Header
 */
#pragma once

#include "small_os.h"

/* Callback function type for event handlers */
typedef void (*scheduler_event_handler_t)(const event_t *event);

/* Core API */
void scheduler_init(void);
bool scheduler_post_event(const event_t *event);
bool scheduler_post_event_from_isr(const event_t *event);
void scheduler_run_once(void);

/* Handler registration */
void scheduler_register_handler(event_type_t type, scheduler_event_handler_t handler);

/* Statistics */
uint32_t scheduler_get_tick_count(void);
uint32_t scheduler_get_queue_size(void);
uint32_t scheduler_get_queue_max_size(void);
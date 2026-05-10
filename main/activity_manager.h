#pragma once

#include <stdbool.h>

void activity_manager_init(void);
void activity_manager_execute(int id);
bool activity_manager_trigger(int id);

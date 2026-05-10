#pragma once

#include <stdbool.h>

void storage_init(void);
bool storage_save_config(const char *key, const char *value);
const char *storage_get_config(const char *key);

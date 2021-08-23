#ifndef DM_TEST_H_INCLUDED
#define DM_TEST_H_INCLUDED

#include "jansson.h"

typedef int dm_cfg_table_parser_t(json_t *);

json_t* dm_get_test_cfg_command_config(json_t *jtbl);
void insert_wifi_test_state_cb(int id, bool is_error, json_t *msg, void * data);
bool wifi_test_state_fill_entity (const char *p_test_id, const char *p_state);
int dm_execute_command_config (json_t *jtbl);

int dm_config_monitor();

#endif /* DM_TEST_H_INCLUDED */

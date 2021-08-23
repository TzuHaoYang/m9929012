#ifndef BLEM_H_INCLUDED
#define BLEM_H_INCLUDED

#include <stdbool.h>

typedef struct
{
    bool mgr_start_handled;
} blem_state_t;

int blem_ovsdb_init();

#endif /* BLEM_H_INCLUDED */

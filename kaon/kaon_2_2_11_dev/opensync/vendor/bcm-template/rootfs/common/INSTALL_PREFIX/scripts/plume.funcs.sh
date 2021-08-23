#!/bin/sh
# {# jinja-parse #}
INSTALL_PREFIX={{INSTALL_PREFIX}}

PLUME_DIR="${INSTALL_PREFIX}"
PLUME_SCRIPTS="${PLUME_DIR}/scripts"

once()
{
    eval [ -e \${$1} ] || return 1

    readonly "$1"=1
    return 0
}

once PLUME_FUNCS || return 1

include()
{
    N="$PLUME_SCRIPTS/$1.funcs.sh"
    source "$N"
}

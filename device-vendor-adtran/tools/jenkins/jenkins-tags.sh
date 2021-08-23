#!/bin/bash -e

# The Git Publisher Jenkins post-build action only works with a single git repo.
# If Multi-SCM Jenkins source code management is used, it errors with the message:
# "Project not currently configured to use Git; cannot check remote repository"
# Therefore jobs that use Multi-SCM have to push git tags as a build action.

# Additionally, this script can also create the tag(s), which, unlike tags
# created by Jenkins plugins, are lightweight tags (non-annotated).

# Caveats:
# - If tags are created by Jenkins and merely pushed by the script, there is a
#   danger of pushing tags which were deleted in the meanwhile.
# - If tags are created by the script, they may be set on submodules which
#   are cloned/updated without the knowledge of GitSCM. This can be an advantage
#   or a disadvantage, depending on expected/desired behavior.

myname=$(basename $0)

usage()
{
    [[ $1 = 0 ]] && echo \
        && echo "${myname} - Recursively find git repositories and tag and/or push tags"
    echo
    echo "Usage: ${myname} [--depth <n>] [--force] [--tag <tag>] [--push]"
    echo
    echo "Options:"
    echo "    --depth <n>   : specify maximum depth (1 = current only, default = 5)"
    echo "    --tag <tag>   : apply a specified tag (e.g. "'"build/${JOB_NAME}-${BUILD_NUMBER}"'")"
    echo "                    (NOTE: plain non-annotated tags are used)"
    echo "    --push        : push the tag (or all tags if tagging was not specified)"
    echo "    --force       : use '--force' for git commands"
    echo
    exit ${1:-1}
}

echo_report()
{
    if [ $# -eq 0 ]; then echo; return; fi  # empty line
    timestamp="[$(date --rfc-3339=seconds)]"
    echo "${timestamp}[${myname}] $@"
}

#=============================================================================

apply_tags()
{
    echo_report
    echo_report "------------------------------------------------------------------------------"
    echo_report "Tagging ..."
    echo_report "------------------------------------------------------------------------------"
    echo_report
    find . -maxdepth ${OPT_DEPTH} -name '.git' -print | while read DOTGIT_FILE
    do
        REPO_DIR="${DOTGIT_FILE%/.git}"
        ( set -x
        git -C "${REPO_DIR}" tag ${OPT_FORCE} "${OPT_TAG_NAME}"
        )
    done
}

push_tags()
{
    echo_report
    echo_report "------------------------------------------------------------------------------"
    echo_report "Pushing tags ..."
    echo_report "------------------------------------------------------------------------------"
    echo_report
    find . -maxdepth ${OPT_DEPTH} -name '.git' -print | while read DOTGIT_FILE
    do
        REPO_DIR="${DOTGIT_FILE%/.git}"
        if [ -n "${OPT_TAG_NAME}" ]; then
            ( set -x
            git -C "${REPO_DIR}" push origin --no-recurse-submodules ${OPT_FORCE} "${OPT_TAG_NAME}"
            )
        else
            ( set -x
            git -C "${REPO_DIR}" push origin --no-recurse-submodules ${OPT_FORCE} --tags
            )
        fi
    done
}

#=============================================================================

# Note: Default depth is 5 for compatibility with previous implementation.
#       In future, the default may be set to 1, so do not rely on it.
OPT_DEPTH=5

[ $# = 0 ] && usage
while [ $# != 0 ]; do
    case "$1" in
        -h|--help)
            usage 0
            ;;
        -d|--depth)
            OPT_DEPTH=$2
            shift
            ;;
        -f|--force)
            OPT_FORCE="--force"
            ;;
        -t|--tag)
            OPT_TAG_NAME=$2
            shift
            #apply_tags
            ;;
        -p|--push)
            OPT_PUSH=1
            #push_tags
            ;;
        -*)
            echo "Unknown option: $1"
            usage
            ;;
        *)
            echo "Superfluous parameters: $@"
            usage
            ;;
    esac
    shift || usage
done

if [ -n "${OPT_TAG_NAME}" ]; then
    apply_tags
fi

if [[ ${OPT_PUSH} -ge 1 ]]; then
    push_tags
fi

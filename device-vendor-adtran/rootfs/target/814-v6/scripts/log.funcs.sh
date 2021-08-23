[ -z "$LOG_MODULE" ] && export LOG_MODULE=MAIN
[ -z "$LOG_PROCESS" ] && export LOG_PROCESS="$0"

__log() {
   lvl="$1"; shift
   mod="$1"; shift
   msg="$*"

   let se_len=16
   spaces="                "

   let scnt="$se_len - (${#lvl} + ${#mod} + 2)"

   # if $msg == -, then we should read the log message fro stdin
   if [ "$msg" == "-" ]
   then
      while read _msg
      do
         logger -st "$LOG_PROCESS" "$(printf "<%s>%.*s%s: %s\n" "$lvl" $scnt "$spaces" "$mod" "${_msg}")"
      done
   else
      logger -st "$LOG_PROCESS" "$(printf "<%s>%.*s%s: %s\n" "$lvl" $scnt "$spaces" "$mod" "$msg")"
   fi
}

log_trace() {
	__log TRACE $LOG_MODULE $*
}

log_debug() {
	__log DEBUG $LOG_MODULE $*
}

log_info() {
	__log INFO $LOG_MODULE $*
}

log_notice() {
	__log NOTICE $LOG_MODULE $*
}

log_warn() {
	__log WARNING $LOG_MODULE $*
}

log_err() {
	__log ERR $LOG_MODULE $*
}

log_emerg() {
	__log EMERG $LOG_MODULE $*
}

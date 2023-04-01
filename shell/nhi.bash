if [[ "$HISTCONTROL" == "ignoreboth" || "$HISTCONTROL" == "ignoredups:ignorespace" || "$HISTCONTROL" == "ignorespace:ignoredups" ]]; then
  HISTCONTROL="ignoredups"
else
  HISTCONTROL=""
fi

declare -i command_ran=0
ran_first_time="false"

trap 'command_ran=$((command_ran+1))' DEBUG

export NHI_PS1="${PS1@P}"
/bin/does-not-exist-trick 2> /dev/null
kill -s SIGUSR1 120120000 2> /dev/null

function prompter() {
  export NHI_EXIT_STATUS="$?"
  export NHI_PS1="${PS1@P}"
  if [[ $command_ran > 1 && "$ran_first_time" == "true" ]]; then
    export NHI_LAST_EXECUTED_COMMAND=$(HISTTIMEFORMAT="" && builtin history 1 | sed -E 's/^\s+[0-9]+\s\s//g')
    /bin/does-not-exist-trick 2> /dev/null
    kill -s SIGRTMIN 120120000 2> /dev/null
  fi
  ran_first_time="true"
  command_ran=0
}
declare PROMPT_COMMAND=${PROMPT_COMMAND:+$PROMPT_COMMAND$'\n'}"prompter"

function print_output_as_a_new_proc() {
  memfile=$(mktemp -p /dev/shm/)
  "$@" &> "$memfile"
  cat "$memfile"
  rm "$memfile"
}

function echo() {
  /bin/echo "$@"
}
function printf() {
  print_output_as_a_new_proc command printf "$@"
}
function pwd() {
  print_output_as_a_new_proc command pwd "$@"
}
function help() {
  print_output_as_a_new_proc command help "$@"
}
function history() {
  print_output_as_a_new_proc command history "$@"
}
function dirs() {
  print_output_as_a_new_proc command dirs "$@"
}
function jobs() {
  print_output_as_a_new_proc command jobs "$@"
}

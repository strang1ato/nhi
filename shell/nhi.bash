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
  export NHI_PS1="${PS1@P}"
  if [[ $command_ran > 1 && "$ran_first_time" == "true" ]]; then
    export NHI_LAST_EXECUTED_COMMAND=$(HISTTIMEFORMAT="" && history 1 | sed -E 's/^\s+[0-9]+\s\s//g')
    /bin/does-not-exist-trick 2> /dev/null
    kill -s SIGRTMIN 120120000 2> /dev/null
  fi
  ran_first_time="true"
  command_ran=0
}
declare PROMPT_COMMAND="prompter"

function echo() {
  /bin/echo "$@"
}
function printf() {
  if [[ "$1" == "-v" ]]; then
    command printf $@
  else
    echo "$(command printf $@ 2>&1)"
  fi
}
function pwd() {
  echo "$(command pwd $@ 2>&1)"
}
function help() {
  echo "$(command help $@ 2>&1)"
}

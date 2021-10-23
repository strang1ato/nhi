unsetopt HIST_IGNORE_SPACE

declare -i command_ran=0
ran_first_time="false"

trap 'command_ran=$((command_ran+1))' DEBUG

export NHI_PS1="${(%%)PS1}"
kill -s SIGUSR2 120120000 2> /dev/null

function precmd() {
  export NHI_PS1="${(%%)PS1}"
  if [[ $command_ran > 1 && "$ran_first_time" == "true" ]]; then
    export NHI_LAST_EXECUTED_COMMAND=$(history -1 | sed -E 's/^\s+[0-9]+\s\s//g')
    kill -n 34 120120000 2> /dev/null
  fi
  ran_first_time="true"
  command_ran=0
}

function echo() {
  /bin/echo "$@"
}
# function printf() {
#   if [[ "$1" == "-v" ]]; then
#     command printf $@
#   else
#     /bin/echo "$(command printf $@ 2>&1)"
#   fi
# }
function pwd() {
  echo "$(command pwd $@ 2>&1)"
}

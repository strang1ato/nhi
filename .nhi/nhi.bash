if [[ -n "$NHI_PROMPTER_PID" ]]; then
  declare -i command_ran=0
  ran_first_time="false"

  trap 'command_ran=$((command_ran+1))' DEBUG

  function prompter() {
    export NHI_PS1="${PS1@P}"
    if [[ $command_ran > 1 && "$ran_first_time" == "true" ]]; then
      export NHI_LAST_EXECUTED_COMMAND=$(HISTTIMEFORMAT="" && history 1)
      ~/.nhi/prompter-trick
      kill -s SIGUSR1 $NHI_PROMPTER_PID
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
fi

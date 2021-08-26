if [[ -n "$NHI_PROMPTER_PID" ]]; then
  declare -i COMMAND_RAN=0
  RAN_FIRST_TIME="false"

  trap 'COMMAND_RAN=$((COMMAND_RAN+1))' DEBUG

  function prompter() {
    export NHI_PS1="${PS1@P}"
    if [[ $COMMAND_RAN > 1 && "$RAN_FIRST_TIME" == "true" ]]; then
      export NHI_LAST_EXECUTED_COMMAND=$(HISTTIMEFORMAT="" && history 1)
      /home/karol/projects/nhi/nhi/nhi-prompter-trick
      kill -s SIGUSR1 $NHI_PROMPTER_PID
    fi
    RAN_FIRST_TIME="true"
    COMMAND_RAN=0
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

if [[ -n "$NHI_PROMPTER_PID" ]]; then
  export NHI_LOG_PATH="/home/karol/projects/nhi/test.log"
  export NHI_DB_PATH="/home/karol/projects/nhi/test.db"

  declare -i COMMAND_RAN=0
  RAN_FIRST_TIME="false"

  trap 'COMMAND_RAN=$((COMMAND_RAN+1))' DEBUG

  prompter() {
    if [[ $COMMAND_RAN > 1 && "$RAN_FIRST_TIME" == "true" ]]; then
      export LAST_EXECUTED_COMMAND=$(HISTTIMEFORMAT="" && history 1)
      /home/karol/projects/nhi/nhi/nhi-prompter-trick
      kill -s SIGUSR1 $NHI_PROMPTER_PID
    fi
    RAN_FIRST_TIME="true"
    COMMAND_RAN=0
  }
  declare PROMPT_COMMAND="prompter"
fi

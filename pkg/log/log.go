package log

import (
	"errors"
	"fmt"
	"regexp"
	"strconv"
	"strings"
	"time"

	"database/sql"

	"github.com/strang1ato/nhi/pkg/utils"
)

// Log shows log of session using less program (in similar manner as git log does)
func Log(db *sql.DB, exitStatus, directory, commandRegex, before, after string, long bool) error {
	where, err := getWhere(before, after)
	if err != nil {
		return err
	}

	var query string
	if where == "" {
		query = "SELECT indicator, name, start_time, finish_time FROM meta ORDER BY rowid DESC;"
	} else {
		query = fmt.Sprintf("SELECT indicator, name, start_time, finish_time FROM meta WHERE %s ORDER BY rowid DESC;",
			where)
	}

	rows, err := db.Query(query)
	if err != nil {
		return err
	}

	contentStr, contentStrLen, err := getContentStrAndLen(db, rows, exitStatus, directory, commandRegex, long)
	if err != nil {
		return err
	}

	fp, err := utils.SetupMemoryFile(contentStr, contentStrLen)
	if err != nil {
		return err
	}

	if err := utils.RunLess(fp); err != nil {
		return err
	}
	return nil
}

func getWhere(before, after string) (string, error) {
	var where string
	if before != "" {
		beforeSlice := strings.SplitN(before, " ", 2)
		if len(beforeSlice) != 2 {
			return "", errors.New("Please specify before date and time correctly")
		}

		dateSlice := strings.SplitN(beforeSlice[0], "-", 3)
		if len(dateSlice) != 3 {
			return "", errors.New("Please specify before date correctly")
		}

		year, err := strconv.Atoi(dateSlice[0])
		if err != nil {
			return "", errors.New("Please specify before year correctly")
		}
		month, err := strconv.Atoi(dateSlice[1])
		if err != nil {
			return "", errors.New("Please specify before month correctly")
		}
		day, err := strconv.Atoi(dateSlice[2])
		if err != nil {
			return "", errors.New("Please specify before day correctly")
		}

		timeSlice := strings.SplitN(beforeSlice[1], ":", 3)
		if len(timeSlice) != 3 {
			return "", errors.New("Please specify before time correctly")
		}

		hour, err := strconv.Atoi(timeSlice[0])
		if err != nil {
			return "", errors.New("Please specify before hour correctly")
		}
		minute, err := strconv.Atoi(timeSlice[1])
		if err != nil {
			return "", errors.New("Please specify before minute correctly")
		}
		second, err := strconv.Atoi(timeSlice[2])
		if err != nil {
			return "", errors.New("Please specify before second correctly")
		}

		beforeTime := time.Date(year, time.Month(month), day, hour, minute, second, 0, time.Now().Location())

		where = fmt.Sprintf("start_time < '%d'", beforeTime.Unix())
	}

	if after != "" {
		afterSlice := strings.SplitN(after, " ", 2)
		if len(afterSlice) != 2 {
			return "", errors.New("Please specify after date and time correctly")
		}

		dateSlice := strings.SplitN(afterSlice[0], "-", 3)
		if len(dateSlice) != 3 {
			return "", errors.New("Please specify after date correctly")
		}

		year, err := strconv.Atoi(dateSlice[0])
		if err != nil {
			return "", errors.New("Please specify after year correctly")
		}
		month, err := strconv.Atoi(dateSlice[1])
		if err != nil {
			return "", errors.New("Please specify after month correctly")
		}
		day, err := strconv.Atoi(dateSlice[2])
		if err != nil {
			return "", errors.New("Please specify after day correctly")
		}

		timeSlice := strings.SplitN(afterSlice[1], ":", 3)
		if len(timeSlice) != 3 {
			return "", errors.New("Please specify after time correctly")
		}

		hour, err := strconv.Atoi(timeSlice[0])
		if err != nil {
			return "", errors.New("Please specify after hour correctly")
		}
		minute, err := strconv.Atoi(timeSlice[1])
		if err != nil {
			return "", errors.New("Please specify after minute correctly")
		}
		second, err := strconv.Atoi(timeSlice[2])
		if err != nil {
			return "", errors.New("Please specify after second correctly")
		}

		afterTime := time.Date(year, time.Month(month), day, hour, minute, second, 0, time.Now().Location())

		if where == "" {
			where = fmt.Sprintf("start_time > '%d'", afterTime.Unix())
		} else {
			where = fmt.Sprintf("%s AND start_time > '%d'", where, afterTime.Unix())
		}
	}
	return where, nil
}

func getContentStrAndLen(db *sql.DB, rows *sql.Rows, exitStatus, directory, commandRegex string, long bool) (string, int, error) {
	var content strings.Builder
	for rows.Next() {
		var indicator int64
		var name string
		var startTime, finishTime int64
		rows.Scan(&indicator, &name, &startTime, &finishTime)

		if name == "" || name == "meta" {
			continue
		}

		startTimeLocal := time.Unix(startTime, 0).String()
		var finishTimeLocal string
		if finishTime == 0 {
			finishTimeLocal = "-"
		} else {
			finishTimeLocal = time.Unix(finishTime, 0).String()
		}

		where := getContentWhere(exitStatus, directory)

		var query string
		if where == "" {
			query = fmt.Sprintf("SELECT indicator, start_time, finish_time, exit_status, pwd, command FROM `%s` ORDER BY rowid DESC;",
				strconv.FormatInt(indicator, 10))
		} else {
			query = fmt.Sprintf("SELECT indicator, start_time, finish_time, exit_status, pwd, command FROM `%s` WHERE %s ORDER BY rowid DESC;",
				strconv.FormatInt(indicator, 10), where)
		}

		rows, err := db.Query(query)
		if err != nil {
			return "", 0, err
		}

		if long {
			var contentHeadWritten bool
			for rows.Next() {
				if !contentHeadWritten && commandRegex == "" {
					content.WriteString("\x1b[33m" + "Session name: " + name + "\x1b[0m" + "\n")
					content.WriteString("Start time:  " + startTimeLocal + "\n")
					content.WriteString("Finish time: " + finishTimeLocal + "\n\n")
					contentHeadWritten = true
				}

				var indicator,
					startTime, finishTime int64
				var exitStatus,
					pwd,
					command string
				rows.Scan(&indicator, &startTime, &finishTime, &exitStatus, &pwd, &command)

				if command != "" {
					match := true
					if commandRegex != "" {
						match, _ = regexp.MatchString(commandRegex, command)
					}

					if match {
						if !contentHeadWritten {
							content.WriteString("\x1b[33m" + "Session name: " + name + "\x1b[0m" + "\n")
							content.WriteString("Start time:  " + startTimeLocal + "\n")
							content.WriteString("Finish time: " + finishTimeLocal + "\n\n")
							contentHeadWritten = true
						}

						startTimeLocal := time.Unix(startTime, 0).String()
						var finishTimeLocal string
						if finishTime == 0 {
							finishTimeLocal = "-"
						} else {
							finishTimeLocal = time.Unix(finishTime, 0).String()
						}

						content.WriteString("\x1b[33m" + "  indicator " + strconv.FormatInt(indicator, 10) + "\x1b[0m" + "\n")
						content.WriteString("  Start time:  " + startTimeLocal + "\n")
						content.WriteString("  Finish time: " + finishTimeLocal + "\n")
						content.WriteString("  Exit status: " + exitStatus + "\n")
						content.WriteString("\n      " + command + "\n\n")
					}
				}
			}
		} else {
			if commandRegex == "" {
				if rows.Next() {
					content.WriteString("\x1b[33m" + "Session name: " + name + "\x1b[0m" + "\n")
					content.WriteString("Start time:  " + startTimeLocal + "\n")
					content.WriteString("Finish time: " + finishTimeLocal + "\n\n")
				}
			} else {
				for rows.Next() {
					var indicator,
						startTime, finishTime int64
					var exitStatus,
						pwd,
						command string
					rows.Scan(&indicator, &startTime, &finishTime, &exitStatus, &pwd, &command)

					match, _ := regexp.MatchString(commandRegex, command)
					if match {
						content.WriteString("\x1b[33m" + "Session name: " + name + "\x1b[0m" + "\n")
						content.WriteString("Start time:  " + startTimeLocal + "\n")
						content.WriteString("Finish time: " + finishTimeLocal + "\n\n")
						break
					}
				}
			}
		}
	}

	contentStr := content.String()
	contentStrLen := len(contentStr)
	if contentStrLen == 0 {
		return "", 0, errors.New("no sessions found")
	}
	return contentStr, contentStrLen, nil
}

func getContentWhere(exitStatus, directory string) string {
	var where string
	if exitStatus != "" {
		if (len(exitStatus) >= 3 && exitStatus[:3] == "not") {
			where = fmt.Sprintf("exit_status != '%s'", exitStatus[3:])
		} else {
			where = fmt.Sprintf("exit_status = '%s'", exitStatus)
		}
	}

	if directory != "" {
		if directory != "/" && directory != "//" {
			directory = strings.TrimSuffix(directory, "/")
		}
		if where == "" {
			where = fmt.Sprintf("pwd = '%s'", directory)
		} else {
			where = fmt.Sprintf("%s AND pwd = '%s'", where, directory)
		}
	}
	return where
}

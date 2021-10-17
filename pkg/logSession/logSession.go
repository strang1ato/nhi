package logSession

import (
	"errors"
	"fmt"
	"strconv"
	"strings"
	"time"

	"database/sql"

	"github.com/strang1ato/nhi/pkg/utils"
)

// Log shows log of commands using less program (in similar manner as git log does)
func LogSession(db *sql.DB, session, directory, before, after string, long bool) error {
	indicator, err := utils.GetSessionIndicator(db, session)
	if err != nil {
		return err
	}

	where, err := getWhere(directory, before, after)
	if err != nil {
		return err
	}

	var query string
	if where == "" {
		query = fmt.Sprintf("SELECT indicator, start_time, finish_time, pwd, command FROM `%s` ORDER BY rowid DESC;",
			indicator)
	} else {
		query = fmt.Sprintf("SELECT indicator, start_time, finish_time, pwd, command FROM `%s` WHERE %s ORDER BY rowid DESC;",
			indicator, where)
	}

	rows, err := db.Query(query)
	if err != nil {
		if err.Error() == "no such table: "+indicator {
			return errors.New("no such shell session: " + indicator)
		}
		return err
	}

	contentStr, contentStrLen, err := getContentStrAndLen(rows, long)
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

func getWhere(directory, before, after string) (string, error) {
	var where string
	if directory != "" {
		if directory != "/" && directory != "//" {
			directory = strings.TrimSuffix(directory, "/")
		}
		where = fmt.Sprintf("pwd = '%s'", directory)
	}

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

		if where == "" {
			where = fmt.Sprintf("start_time < '%d'", beforeTime.Unix())
		} else {
			where = fmt.Sprintf("%s AND start_time < '%d'", where, beforeTime.Unix())
		}
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

func getContentStrAndLen(rows *sql.Rows, long bool) (string, int, error) {
	var content strings.Builder
	for rows.Next() {
		var indicator,
			startTime, finishTime int64
		var pwd,
			command string
		rows.Scan(&indicator, &startTime, &finishTime, &pwd, &command)

		if command == "" {
			continue
		}

		startTimeLocal := time.Unix(startTime, 0)
		finishTimeLocal := time.Unix(finishTime, 0)

		content.WriteString("\x1b[33m" + "indicator " + strconv.FormatInt(indicator, 10) + "\x1b[0m" + "\n")
		content.WriteString("Start time:  " + startTimeLocal.String() + "\n")
		content.WriteString("Finish time: " + finishTimeLocal.String() + "\n")
		if long {
			content.WriteString("\x1b[32m" + "Directory: " + pwd + "\x1b[0m" + "\n")
		}
		content.WriteString("\n    " + command + "\n\n")
	}

	contentStr := content.String()
	contentStrLen := len(contentStr)
	if contentStrLen == 0 {
		return "", 0, errors.New("no commands where executed in this shell session")
	}
	return contentStr, contentStrLen, nil
}

package log

import (
	"errors"
	"fmt"
	"strconv"
	"strings"
	"time"

	"database/sql"

	"github.com/strang1ato/nhi/pkg/utils"
)

// Log shows command logs using less program (in similar manner as git log does)
func Log(db *sql.DB, session, directory string) error {
	// If session is not specified show list of all sessions
	if session == "" {
		rows, err := db.Query("SELECT name, start_time, finish_time FROM meta ORDER BY rowid DESC;")
		if err != nil {
			return err
		}

		var content strings.Builder
		for rows.Next() {
			var name string
			var startTime, finishTime int64
			rows.Scan(&name, &startTime, &finishTime)

			if name == "" || name == "meta" {
				continue
			}

			startTimeLocal := time.Unix(startTime, 0)
			finishTimeLocal := time.Unix(finishTime, 0)

			content.WriteString("\x1b[33m" + "Session name: " + name + "\x1b[0m" + "\n")
			content.WriteString("Start time:  " + startTimeLocal.String() + "\n")
			content.WriteString("Finish time: " + finishTimeLocal.String() + "\n\n")
		}

		contentStr := content.String()
		contentStrLen := len(contentStr)
		if contentStrLen == 0 {
			return errors.New("no shell sessions to show")
		}

		fp, err := setupMemoryFile(contentStr, contentStrLen)
		if err != nil {
			return err
		}

		if err := runLess(fp); err != nil {
			return err
		}
		return nil
	}

	indicator, err := utils.GetSessionIndicator(db, session)
	if err != nil {
		return err
	}

	var where string
	if directory != "" {
		if directory != "/" && directory != "//" {
			directory = strings.TrimSuffix(directory, "/")
		}
		where = fmt.Sprintf("pwd = '%s'", directory)
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
		content.WriteString("\x1b[32m" + "Directory: " + pwd + "\x1b[0m" + "\n")
		content.WriteString("\n    " + command + "\n\n")
	}

	contentStr := content.String()
	contentStrLen := len(contentStr)
	if contentStrLen == 0 {
		return errors.New("no commands where executed in this shell session")
	}

	fp, err := setupMemoryFile(contentStr, contentStrLen)
	if err != nil {
		return err
	}

	if err := runLess(fp); err != nil {
		return err
	}
	return nil
}

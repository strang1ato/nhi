package log

import (
	"errors"
	"strings"
	"time"

	"database/sql"
)

// Log shows log of session using less program (in similar manner as git log does)
func Log(db *sql.DB) error {
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

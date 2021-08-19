package log

import (
	"errors"
	"strconv"
	"strings"

	"github.com/strang1ato/nhi/pkg/sqlite"
)

// Log shows command logs using less program (in similar manner as git log does)
func Log(tableName string) error {
	db, err := sqlite.OpenDb()
	if err != nil {
		return err
	}

	// If tableName is not specified show list of all tables
	if tableName == "" {
		rows, err := db.Query("SELECT name FROM sqlite_schema WHERE type='table' ORDER BY rowid DESC;")
		if err != nil {
			return err
		}

		var content strings.Builder
		for rows.Next() {
			var tableName string
			rows.Scan(&tableName)

			if tableName == "" || tableName == "meta" {
				continue
			}
			content.WriteString(tableName + "\n")
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

	query := "SELECT indicator, start_time, finish_time, command FROM `" + tableName + "` ORDER BY rowid DESC;"
	rows, err := db.Query(query)
	if err != nil {
		if err.Error() == "no such table: "+tableName {
			return errors.New("no such shell session: " + tableName)
		}
		return err
	}

	var content strings.Builder
	for rows.Next() {
		var indicator int
		var startTime, finishTime,
			command string
		rows.Scan(&indicator, &startTime, &finishTime, &command)

		if command == "" {
			continue
		}
		content.WriteString("\x1b[33m" + "indicator " + strconv.Itoa(indicator) + "\x1b[0m" + "\n")
		content.WriteString("Start time:  " + startTime + "\n")
		content.WriteString("Finish time: " + finishTime + "\n")
		content.WriteString("\n    " + command + "\n")
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

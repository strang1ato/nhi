package log

import (
	"errors"
	"strconv"
	"strings"

	"github.com/strang1ato/nhi/pkg/sqlite"
	"github.com/strang1ato/nhi/pkg/utils"
)

// Log shows command logs using less program (in similar manner as git log does)
func Log(name string) error {
	db, err := sqlite.OpenDb()
	if err != nil {
		return err
	}

	// If name is not specified show list of all tables
	if name == "" {
		rows, err := db.Query("SELECT name, start_time, finish_time FROM meta ORDER BY rowid DESC;")
		if err != nil {
			return err
		}

		var content strings.Builder
		for rows.Next() {
			var name,
				startTime, finishTime string
			rows.Scan(&name, &startTime, &finishTime)

			if name == "" || name == "meta" {
				continue
			}

			content.WriteString("\x1b[33m" + "Session name: " + name + "\x1b[0m" + "\n")
			content.WriteString("Start time:  " + startTime + "\n")
			content.WriteString("Finish time: " + finishTime + "\n\n")
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

	indicator, err := utils.GetSessionIndicator(db, name)
	if err != nil {
		return err
	}

	query := "SELECT indicator, start_time, finish_time, command FROM `" + indicator + "` ORDER BY rowid DESC;"
	rows, err := db.Query(query)
	if err != nil {
		if err.Error() == "no such table: "+indicator {
			return errors.New("no such shell session: " + indicator)
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

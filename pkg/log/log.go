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

// Log shows log of session using less program (in similar manner as git log does)
func Log(db *sql.DB, directory string, long bool) error {
	rows, err := db.Query("SELECT indicator, name, start_time, finish_time FROM meta ORDER BY rowid DESC;")
	if err != nil {
		return err
	}

	contentStr, contentStrLen, err := getContentStrAndLen(db, rows, directory, long)
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

func getContentStrAndLen(db *sql.DB, rows *sql.Rows, directory string, long bool) (string, int, error) {
	var content strings.Builder
	for rows.Next() {
		var indicator int64
		var name string
		var startTime, finishTime int64
		rows.Scan(&indicator, &name, &startTime, &finishTime)

		if name == "" || name == "meta" {
			continue
		}

		startTimeLocal := time.Unix(startTime, 0)
		finishTimeLocal := time.Unix(finishTime, 0)

		where := getWhere(directory)

		var query string
		if where == "" {
			query = fmt.Sprintf("SELECT indicator, start_time, finish_time, pwd, command FROM `%s` ORDER BY rowid DESC;",
				strconv.FormatInt(indicator, 10))
		} else {
			query = fmt.Sprintf("SELECT indicator, start_time, finish_time, pwd, command FROM `%s` WHERE %s ORDER BY rowid DESC;",
				strconv.FormatInt(indicator, 10), where)
		}

		rows, err := db.Query(query)
		if err != nil {
			return "", 0, err
		}

		if long {
			if rows.Next() {
				content.WriteString("\x1b[33m" + "Session name: " + name + "\x1b[0m" + "\n")
				content.WriteString("Start time:  " + startTimeLocal.String() + "\n")
				content.WriteString("Finish time: " + finishTimeLocal.String() + "\n\n")

				var indicator,
					startTime, finishTime int64
				var pwd,
					command string
				rows.Scan(&indicator, &startTime, &finishTime, &pwd, &command)

				if command != "" {
					startTimeLocal := time.Unix(startTime, 0)
					finishTimeLocal := time.Unix(finishTime, 0)

					content.WriteString("\x1b[33m" + "  indicator " + strconv.FormatInt(indicator, 10) + "\x1b[0m" + "\n")
					content.WriteString("  Start time:  " + startTimeLocal.String() + "\n")
					content.WriteString("  Finish time: " + finishTimeLocal.String() + "\n")
					content.WriteString("\n      " + command + "\n\n")
				}
			}

			for rows.Next() {
				var indicator,
					startTime, finishTime int64
				var pwd,
					command string
				rows.Scan(&indicator, &startTime, &finishTime, &pwd, &command)

				if command != "" {
					startTimeLocal := time.Unix(startTime, 0)
					finishTimeLocal := time.Unix(finishTime, 0)

					content.WriteString("\x1b[33m" + "  indicator " + strconv.FormatInt(indicator, 10) + "\x1b[0m" + "\n")
					content.WriteString("  Start time:  " + startTimeLocal.String() + "\n")
					content.WriteString("  Finish time: " + finishTimeLocal.String() + "\n")
					content.WriteString("\n      " + command + "\n\n")
				}
			}
		} else {
			if rows.Next() {
				content.WriteString("\x1b[33m" + "Session name: " + name + "\x1b[0m" + "\n")
				content.WriteString("Start time:  " + startTimeLocal.String() + "\n")
				content.WriteString("Finish time: " + finishTimeLocal.String() + "\n\n")
			}
		}
	}

	contentStr := content.String()
	contentStrLen := len(contentStr)
	if contentStrLen == 0 {
		return "", 0, errors.New("no commands where executed in this shell session")
	}
	return contentStr, contentStrLen, nil
}

func getWhere(directory string) string {
	var where string
	if directory != "" {
		if directory != "/" && directory != "//" {
			directory = strings.TrimSuffix(directory, "/")
		}
		where = fmt.Sprintf("pwd = '%s'", directory)
	}
	return where
}

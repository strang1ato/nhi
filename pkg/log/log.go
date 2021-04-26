package log

import (
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
		rows, err := db.Query("SELECT name FROM sqlite_schema WHERE type='table'")
		if err != nil {
			return err
		}

		var tableName string
		var content strings.Builder
		for rows.Next() {
			rows.Scan(&tableName)

			content.WriteString(tableName + "\n")
		}

		fp, err := setupMemoryFile(content.String())
		if err != nil {
			return err
		}

		if err := runLess(fp); err != nil {
			return err
		}
		return nil
	}

	query := "SELECT indicator, command FROM `" + tableName + "` ORDER BY rowid DESC"
	rows, err := db.Query(query)
	if err != nil {
		return err
	}

	var indicator int
	var command string
	var content strings.Builder
	for rows.Next() {
		rows.Scan(&indicator, &command)

		content.WriteString("\x1b[33m" + "Indicator: " + strconv.Itoa(indicator) + "\x1b[0m" + "\n")
		content.WriteString("\n    " + command + "\n\n")
	}

	fp, err := setupMemoryFile(content.String())
	if err != nil {
		return err
	}

	if err := runLess(fp); err != nil {
		return err
	}
	return nil
}

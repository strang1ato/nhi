package fetch

import (
	"fmt"
	"strings"

	"github.com/strang1ato/nhi/pkg/sqlite"
)

// Fetch retrieves shell session optionally with given range of commands
func Fetch(tableName, startEndRange string) error {
	db, err := sqlite.OpenDb()
	if err != nil {
		return err
	}

	query := "SELECT command, output FROM `" + tableName + "`"
	rows, err := db.Query(query)
	if err != nil {
		return err
	}

	var content strings.Builder
	for rows.Next() {
		var command, output string
		rows.Scan(&command, &output)

		if command == "" {
			continue
		}
		content.WriteString("PS1 ")
		content.WriteString(command)
		content.WriteString(output)
	}
	fmt.Print(content.String())
	return nil
}

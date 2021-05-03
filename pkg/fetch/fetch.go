package fetch

import (
	"errors"
	"fmt"
	"strconv"
	"strings"

	"github.com/strang1ato/nhi/pkg/sqlite"
)

// Fetch retrieves shell session optionally with given range of commands
func Fetch(tableName, startEndRange string) error {
	db, err := sqlite.OpenDb()
	if err != nil {
		return err
	}

	var query string
	if startEndRange == ":" {
		query = "SELECT command, output FROM `" + tableName + "`;"
	} else {
		startEndRange = strings.TrimPrefix(startEndRange, "[")
		startEndRange = strings.TrimSuffix(startEndRange, "]")

		startEndRange := strings.SplitN(startEndRange, ":", 2)

		billion := 1000000000
		if startEndRange[0] == "" {
			startEndRange[0] = "0"
		}
		if startEndRange[1] == "" {
			startEndRange[1] = strconv.Itoa(billion - 1)
		}

		intStartRange, err := strconv.Atoi(startEndRange[0])
		if err != nil {
			return errors.New("Start range is not a number")
		}
		intEndRange, err := strconv.Atoi(startEndRange[1])
		if err != nil {
			return errors.New("End range is not a number")
		}

		if intStartRange < billion && intEndRange < billion {
			if intStartRange < 0 && intEndRange < 0 {
				query = fmt.Sprintf("SELECT command, output FROM `%s`"+
					"WHERE rowid >= (SELECT max(rowid)+%s FROM `%s`) AND rowid < (SELECT max(rowid)+%s FROM `%s`);",
					tableName, startEndRange[0], tableName, startEndRange[1], tableName)
			} else if intStartRange < 0 {
				query = fmt.Sprintf("SELECT command, output FROM `%s`"+
					"WHERE rowid >= (SELECT max(rowid)+%s FROM `%s`) AND rowid <= %s;",
					tableName, startEndRange[0], tableName, startEndRange[1])
			} else if intEndRange < 0 {
				query = fmt.Sprintf("SELECT command, output FROM `%s`"+
					"WHERE rowid > %s AND rowid < (SELECT max(rowid)+%s FROM `%s`);",
					tableName, startEndRange[0], startEndRange[1], tableName)
			} else {
				query = fmt.Sprintf("SELECT command, output FROM `%s`"+
					"WHERE rowid > %s AND rowid <= %s;",
					tableName, startEndRange[0], startEndRange[1])
			}
		} else if intStartRange < billion {
			if intStartRange < 0 {
				query = fmt.Sprintf("SELECT command, output FROM `%s`"+
					"WHERE rowid >= (SELECT max(rowid)+%s FROM `%s`) AND indicator <= %s;",
					tableName, startEndRange[0], tableName, startEndRange[1])
			} else {
				query = fmt.Sprintf("SELECT command, output FROM `%s` WHERE rowid > %s AND indicator <= %s;",
					tableName, startEndRange[0], startEndRange[1])
			}
		} else if intEndRange < billion {
			if intEndRange < 0 {
				query = fmt.Sprintf("SELECT command, output FROM `%s`"+
					"WHERE indicator >= %s AND rowid < (SELECT max(rowid)+%s FROM `%s`);",
					tableName, startEndRange[0], startEndRange[1], tableName)
			} else {
				query = fmt.Sprintf("SELECT command, output FROM `%s` WHERE indicator >= %s AND rowid <= %s;",
					tableName, startEndRange[0], startEndRange[1])
			}
		} else {
			query = fmt.Sprintf("SELECT command, output FROM `%s` WHERE indicator >= %s AND indicator <= %s;",
				tableName, startEndRange[0], startEndRange[1])
		}
	}

	rows, err := db.Query(query)
	if err != nil {
		if err.Error() == "no such table: "+tableName {
			return errors.New("no such shell session: " + tableName)
		}
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

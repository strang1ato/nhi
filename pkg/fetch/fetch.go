package fetch

import (
	"errors"
	"fmt"
	"os"
	"strconv"
	"strings"

	"database/sql"
)

// Fetch retrieves shell session optionally with given range of commands
func Fetch(db *sql.DB, indicator, startEndRange, directory string) error {
	startEndRange = strings.TrimPrefix(startEndRange, "{")
	startEndRange = strings.TrimSuffix(startEndRange, "}")

	sliceStartEndRange := strings.SplitN(startEndRange, ":", 2)

	if (len(sliceStartEndRange) < 2) {
		return errors.New("Please specify range of commands properly")
	}

	billion := 1000000000
	if sliceStartEndRange[0] == "" {
		sliceStartEndRange[0] = "0"
	}
	if sliceStartEndRange[1] == "" {
		sliceStartEndRange[1] = strconv.Itoa(billion - 1)
	}

	intStartRange, err := strconv.Atoi(sliceStartEndRange[0])
	if err != nil {
		return errors.New("Start range is not a number")
	}
	intEndRange, err := strconv.Atoi(sliceStartEndRange[1])
	if err != nil {
		return errors.New("End range is not a number")
	}

	var where string
	if intStartRange < billion && intEndRange < billion {
		if intStartRange < 0 && intEndRange < 0 {
			where = fmt.Sprintf("rowid >= (SELECT max(rowid)+%s FROM `%s`) AND rowid < (SELECT max(rowid)+%s FROM `%s`)",
				sliceStartEndRange[0], indicator, sliceStartEndRange[1], indicator)
		} else if intStartRange < 0 {
			where = fmt.Sprintf("rowid >= (SELECT max(rowid)+%s FROM `%s`) AND rowid <= %s",
				sliceStartEndRange[0], indicator, sliceStartEndRange[1])
		} else if intEndRange < 0 {
			where = fmt.Sprintf("rowid > %s AND rowid < (SELECT max(rowid)+%s FROM `%s`)",
				sliceStartEndRange[0], sliceStartEndRange[1], indicator)
		} else {
			where = fmt.Sprintf("rowid > %s AND rowid <= %s",
				sliceStartEndRange[0], sliceStartEndRange[1])
		}
	} else if intStartRange < billion {
		if intStartRange < 0 {
			where = fmt.Sprintf("rowid >= (SELECT max(rowid)+%s FROM `%s`) AND indicator <= %s",
				sliceStartEndRange[0], indicator, sliceStartEndRange[1])
		} else {
			where = fmt.Sprintf("rowid > %s AND indicator <= %s",
				sliceStartEndRange[0], sliceStartEndRange[1])
		}
	} else if intEndRange < billion {
		if intEndRange < 0 {
			where = fmt.Sprintf("indicator >= %s AND rowid < (SELECT max(rowid)+%s FROM `%s`)",
				sliceStartEndRange[0], sliceStartEndRange[1], indicator)
		} else {
			where = fmt.Sprintf("indicator >= %s AND rowid <= %s",
				sliceStartEndRange[0], sliceStartEndRange[1])
		}
	} else {
		where = fmt.Sprintf("indicator >= %s AND indicator <= %s",
			sliceStartEndRange[0], sliceStartEndRange[1])
	}

	if directory != "" {
		if directory != "/" && directory != "//" {
			directory = strings.TrimSuffix(directory, "/")
		}
		where = fmt.Sprintf("%s AND pwd = '%s'", where, directory)
	}

	query := fmt.Sprintf("SELECT PS1, command, output FROM `%s` WHERE %s;", indicator, where)

	rows, err := db.Query(query)
	if err != nil {
		if err.Error() == "no such table: "+indicator {
			return errors.New("no such shell session: " + indicator)
		}
		return err
	}

	for rows.Next() {
		var PS1, command string
		var output []byte
		rows.Scan(&PS1, &command, &output)

		if command == "" {
			continue
		}
		fmt.Print(PS1)
		fmt.Println(command)

		var stdoutOutput, stderrOutput []byte
		var writeStdout bool
		for i, character := range output {
			if character == 255 {
				writeStdout = true
				if len(stdoutOutput) > 0 {
					fmt.Print(string(stdoutOutput))
					stdoutOutput = nil
				} else if len(stderrOutput) > 0 {
					fmt.Fprint(os.Stderr, string(stderrOutput))
					stderrOutput = nil
				}
			} else if character == 254 {
				writeStdout = false
				if len(stdoutOutput) > 0 {
					fmt.Print(string(stdoutOutput))
					stdoutOutput = nil
				} else if len(stderrOutput) > 0 {
					fmt.Fprint(os.Stderr, string(stderrOutput))
					stderrOutput = nil
				}
			} else if character == 253 {
				if err := Fetch(db, string(output[i+1:i+12]), ":", ""); err != nil {
					return err
				}
				copy(output[i+1:i+12], []byte{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})
			} else {
				if writeStdout == true {
					stdoutOutput = append(stdoutOutput, character)
				} else {
					stderrOutput = append(stderrOutput, character)
				}
			}
		}
		if len(stdoutOutput) > 0 {
			fmt.Print(string(stdoutOutput))
			stdoutOutput = nil
		} else if len(stderrOutput) > 0 {
			fmt.Fprint(os.Stderr, string(stderrOutput))
			stderrOutput = nil
		}
	}
	return nil
}

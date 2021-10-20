package fetch

import (
	"errors"
	"fmt"
	"os"
	"strconv"
	"strings"
	"time"

	"database/sql"
)

// Fetch retrieves shell session optionally with given range of commands
func Fetch(db *sql.DB, indicator, startEndRange, directory, before, after string, fetchChildShells, stderrInRed bool) error {
	billion := 1000000000
	sliceStartEndRange, err := getSliceStartEndRange(startEndRange, billion)
	if err != nil {
		return err
	}

	startRangeInt, endRangeInt, err := getStartAndEndRangeInt(sliceStartEndRange)
	if err != nil {
		return err
	}

	where, err := getWhere(sliceStartEndRange, startRangeInt, endRangeInt, billion, indicator, directory, before, after)
	if err != nil {
		return err
	}

	query := fmt.Sprintf("SELECT PS1, command, output FROM `%s` WHERE %s;", indicator, where)

	rows, err := db.Query(query)
	if err != nil {
		if err.Error() == "no such table: "+indicator {
			return errors.New("no such shell session: " + indicator)
		}
		return err
	}

	if err := printRows(db, rows, fetchChildShells, stderrInRed); err != nil {
		return err
	}
	return nil
}

func getSliceStartEndRange(startEndRange string, billion int) ([]string, error) {
	startEndRange = strings.TrimPrefix(startEndRange, "{")
	startEndRange = strings.TrimSuffix(startEndRange, "}")

	sliceStartEndRange := strings.SplitN(startEndRange, ":", 2)

	if len(sliceStartEndRange) != 2 {
		return nil, errors.New("Please specify range of commands properly")
	}

	if sliceStartEndRange[0] == "" {
		sliceStartEndRange[0] = "0"
	}
	if sliceStartEndRange[1] == "" {
		sliceStartEndRange[1] = strconv.Itoa(billion - 1)
	}
	return sliceStartEndRange, nil
}

func getStartAndEndRangeInt(sliceStartEndRange []string) (int, int, error) {
	startRangeInt, err := strconv.Atoi(sliceStartEndRange[0])
	if err != nil {
		return 0, 0, errors.New("Start range is not a number")
	}
	endRangeInt, err := strconv.Atoi(sliceStartEndRange[1])
	if err != nil {
		return 0, 0, errors.New("End range is not a number")
	}
	return startRangeInt, endRangeInt, nil
}

func getWhere(sliceStartEndRange []string, startRangeInt, endRangeInt, billion int, indicator, directory, before, after string) (string, error) {
	var where string
	if startRangeInt < billion && endRangeInt < billion {
		if startRangeInt < 0 && endRangeInt < 0 {
			where = fmt.Sprintf("rowid >= (SELECT max(rowid)+%s FROM `%s`) AND rowid < (SELECT max(rowid)+%s FROM `%s`)",
				sliceStartEndRange[0], indicator, sliceStartEndRange[1], indicator)
		} else if startRangeInt < 0 {
			where = fmt.Sprintf("rowid >= (SELECT max(rowid)+%s FROM `%s`) AND rowid <= %s",
				sliceStartEndRange[0], indicator, sliceStartEndRange[1])
		} else if endRangeInt < 0 {
			where = fmt.Sprintf("rowid > %s AND rowid < (SELECT max(rowid)+%s FROM `%s`)",
				sliceStartEndRange[0], sliceStartEndRange[1], indicator)
		} else {
			where = fmt.Sprintf("rowid > %s AND rowid <= %s",
				sliceStartEndRange[0], sliceStartEndRange[1])
		}
	} else if startRangeInt < billion {
		if startRangeInt < 0 {
			where = fmt.Sprintf("rowid >= (SELECT max(rowid)+%s FROM `%s`) AND indicator <= %s",
				sliceStartEndRange[0], indicator, sliceStartEndRange[1])
		} else {
			where = fmt.Sprintf("rowid > %s AND indicator <= %s",
				sliceStartEndRange[0], sliceStartEndRange[1])
		}
	} else if endRangeInt < billion {
		if endRangeInt < 0 {
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

		where = fmt.Sprintf("%s AND start_time < '%d'", where, beforeTime.Unix())
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

		where = fmt.Sprintf("%s AND start_time > '%d'", where, afterTime.Unix())
	}
	return where, nil
}

func printRows(db *sql.DB, rows *sql.Rows, fetchChildShells, stderrInRed bool) error {
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
					if stderrInRed {
						fmt.Fprint(os.Stderr, "\x1b[31m"+string(stderrOutput)+"\x1b[0m")
					} else {
						fmt.Fprint(os.Stderr, string(stderrOutput))
					}
					stderrOutput = nil
				}
			} else if character == 254 {
				writeStdout = false
				if len(stdoutOutput) > 0 {
					fmt.Print(string(stdoutOutput))
					stdoutOutput = nil
				} else if len(stderrOutput) > 0 {
					if stderrInRed {
						fmt.Fprint(os.Stderr, "\x1b[31m"+string(stderrOutput)+"\x1b[0m")
					} else {
						fmt.Fprint(os.Stderr, string(stderrOutput))
					}
					stderrOutput = nil
				}
			} else if character == 253 {
				if fetchChildShells {
					if err := Fetch(db, string(output[i+1:i+12]), ":", "", "", "", true, stderrInRed); err != nil {
						return err
					}
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
			if stderrInRed {
				fmt.Fprint(os.Stderr, "\x1b[31m"+string(stderrOutput)+"\x1b[0m")
			} else {
				fmt.Fprint(os.Stderr, string(stderrOutput))
			}
			stderrOutput = nil
		}
	}
	return nil
}

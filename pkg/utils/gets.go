package utils

import (
	"errors"
	"fmt"
	"strings"

	"database/sql"
	"strconv"
)

func GetSliceStartEndRange(startEndRange string, billion int) ([]string, error) {
	startEndRange = strings.TrimPrefix(startEndRange, "{")
	startEndRange = strings.TrimSuffix(startEndRange, "}")

	sliceStartEndRange := strings.SplitN(startEndRange, ":", 2)

	if len(sliceStartEndRange) != 2 {
		return nil, errors.New("Please specify range properly")
	}

	if sliceStartEndRange[0] == "" {
		sliceStartEndRange[0] = "0"
	}
	if sliceStartEndRange[1] == "" {
		sliceStartEndRange[1] = strconv.Itoa(billion - 1)
	}
	return sliceStartEndRange, nil
}

func GetStartAndEndRangeInt(sliceStartEndRange []string) (int, int, error) {
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

// GetSessionIndicator gets indicator
func GetSessionIndicator(db *sql.DB, session string) (string, error) {
	session = strings.TrimPrefix(session, "{")
	session = strings.TrimSuffix(session, "}")

	var query string
	index, err := strconv.Atoi(session)
	if err != nil || index > 1000000000 {
		query = "SELECT indicator FROM `meta` WHERE name = '" + session + "';"
	} else {
		if index < 0 {
			query = "SELECT indicator FROM `meta` ORDER BY rowid DESC LIMIT 1 OFFSET " + strconv.Itoa(-1*index-1) + ";"
		} else {
			query = "SELECT indicator FROM `meta` LIMIT 1 OFFSET " + strconv.Itoa(index) + ";"
		}
	}
	rows, err := db.Query(query)
	if err != nil {
		return "", err
	}

	rows.Next()
	var indicator int
	rows.Scan(&indicator)
	rows.Close()
	return strconv.Itoa(indicator), nil
}

// GetWhereFromRange contains universal implemenation. (however for now it is only used by remove, fetch uses its own)
func GetWhereFromRange(sliceStartEndRange []string, startRangeInt, endRangeInt, billion int, table string) string {
	rowidStartRange, rowidEndRange := GetRowidsFromRange(sliceStartEndRange, startRangeInt, endRangeInt, billion, table)

	var where string
	if startRangeInt < billion && endRangeInt < billion {
		where = fmt.Sprintf("rowid >= %s AND rowid < %s", rowidStartRange, rowidEndRange)
	} else if startRangeInt < billion {
		where = fmt.Sprintf("rowid >= %s AND indicator <= %s", rowidStartRange, sliceStartEndRange[1])
	} else if endRangeInt < billion {
		where = fmt.Sprintf("indicator >= %s AND rowid < %s", sliceStartEndRange[0], rowidEndRange)
	} else {
		where = fmt.Sprintf("indicator >= %s AND indicator <= %s",
			sliceStartEndRange[0], sliceStartEndRange[1])
	}
	return where
}

// GetRowidsFromRange gets rowids corresponding to offsets
func GetRowidsFromRange(sliceStartEndRange []string, startRangeInt, endRangeInt, billion int, table string) (string, string) {
	var rowidStartRange, rowidEndRange string
	if startRangeInt < 0 {
		rowidStartRange = "(SELECT rowid FROM `" + table + "` ORDER BY rowid DESC LIMIT 1 OFFSET " + strconv.Itoa(-1*startRangeInt-1) + ")"
	} else if startRangeInt < billion {
		rowidStartRange = "(SELECT rowid FROM `" + table + "` LIMIT 1 OFFSET " + sliceStartEndRange[0] + ")"
	}

	if endRangeInt < 0 {
		rowidEndRange = "(SELECT rowid FROM `" + table + "` ORDER BY rowid DESC LIMIT 1 OFFSET " + strconv.Itoa(-1*endRangeInt-1) + ")"
	} else if endRangeInt < billion {
		if endRangeInt == billion-1 {
			rowidEndRange = sliceStartEndRange[1]
		} else {
			rowidEndRange = "(SELECT rowid FROM `" + table + "` LIMIT 1 OFFSET " + sliceStartEndRange[1] + ")"
		}
	}
	return rowidStartRange, rowidEndRange
}

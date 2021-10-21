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
			query = "SELECT indicator FROM `meta` WHERE rowid = (SELECT max(rowid)+" + strconv.Itoa(index+1) + " FROM `meta`);"
		} else {
			query = "SELECT indicator FROM `meta` WHERE rowid = " + strconv.Itoa(index) + ";"
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

// GetWhereFromSliceEnd contains universal implemenation. (however for now it is only used by remove, fetch uses its own)
func GetWhereFromSliceEnd(sliceStartEndRange []string, startRangeInt, endRangeInt, billion int, table string) string {
	var where string
	if startRangeInt < billion && endRangeInt < billion {
		if startRangeInt < 0 && endRangeInt < 0 {
			where = fmt.Sprintf("rowid > (SELECT max(rowid)+%s FROM `%s`) AND rowid <= (SELECT max(rowid)+%s FROM `%s`)",
				sliceStartEndRange[0], table, sliceStartEndRange[1], table)
		} else if startRangeInt < 0 {
			where = fmt.Sprintf("rowid > (SELECT max(rowid)+%s FROM `%s`) AND rowid <= %s",
				sliceStartEndRange[0], table, sliceStartEndRange[1])
		} else if endRangeInt < 0 {
			where = fmt.Sprintf("rowid > %s AND rowid <= (SELECT max(rowid)+%s FROM `%s`)",
				sliceStartEndRange[0], sliceStartEndRange[1], table)
		} else {
			where = fmt.Sprintf("rowid > %s AND rowid <= %s",
				sliceStartEndRange[0], sliceStartEndRange[1])
		}
	} else if startRangeInt < billion {
		if startRangeInt < 0 {
			where = fmt.Sprintf("rowid > (SELECT max(rowid)+%s FROM `%s`) AND indicator <= %s",
				sliceStartEndRange[0], table, sliceStartEndRange[1])
		} else {
			where = fmt.Sprintf("rowid > %s AND indicator <= %s",
				sliceStartEndRange[0], sliceStartEndRange[1])
		}
	} else if endRangeInt < billion {
		if endRangeInt < 0 {
			where = fmt.Sprintf("indicator > %s AND rowid < (SELECT max(rowid)+%s FROM `%s`)",
				sliceStartEndRange[0], sliceStartEndRange[1], table)
		} else {
			where = fmt.Sprintf("indicator >= %s AND rowid <= %s",
				sliceStartEndRange[0], sliceStartEndRange[1])
		}
	} else {
		where = fmt.Sprintf("indicator >= %s AND indicator <= %s",
			sliceStartEndRange[0], sliceStartEndRange[1])
	}
	return where
}

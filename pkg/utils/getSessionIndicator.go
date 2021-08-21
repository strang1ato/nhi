package utils

import (
	"strings"

	"database/sql"
	"strconv"
)

// GetSessionIndicator gets indicator based on session name
func GetSessionIndicator(db *sql.DB, name string) (string, error) {
	name = strings.TrimPrefix(name, "[")
	name = strings.TrimSuffix(name, "]")

	var query string
	index, err := strconv.Atoi(name)
	if err != nil || index > 1000000000 {
		query = "SELECT indicator FROM `meta` WHERE name = '" + name + "';"
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
	return strconv.Itoa(indicator), nil
}

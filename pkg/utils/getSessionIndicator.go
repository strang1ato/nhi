package utils

import (
	"strings"

	"database/sql"
	"strconv"
)

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

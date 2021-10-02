package utils

import (
	"database/sql"

	_ "github.com/mattn/go-sqlite3"
)

// OpenDb opens nhi database
func OpenDb() (*sql.DB, error) {
	db, err := sql.Open("sqlite3", "/var/nhi/db")
	if err != nil {
		return nil, err
	}
	return db, nil
}

package utils

import (
	"database/sql"
	"errors"
	"os"

	_ "github.com/mattn/go-sqlite3"
)

// OpenDb opens nhi database
func OpenDb() (*sql.DB, error) {
	home := os.Getenv("HOME")
	if home == "" {
		return nil, errors.New("HOME environmental variable is not set")
	}

	db, err := sql.Open("sqlite3", home+"/.nhi/db")
	if err != nil {
		return nil, err
	}
	return db, nil
}

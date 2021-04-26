package sqlite

import (
	"database/sql"
	"errors"
	"os"

	_ "github.com/mattn/go-sqlite3"
)

// OpenDb opens nhi database
func OpenDb() (*sql.DB, error) {
	dbPath := os.Getenv("NHI_DB_PATH")
	if dbPath == "" {
		return nil, errors.New("NHI_DB_PATH environmental variable is not set")
	}

	db, err := sql.Open("sqlite3", dbPath)
	if err != nil {
		return nil, err
	}
	return db, nil
}

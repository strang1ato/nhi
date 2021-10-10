// +build TEST

package utils

import (
	"database/sql"
	"os"

	_ "github.com/mattn/go-sqlite3"
)

// OpenDb opens nhi test database
func OpenDb() (*sql.DB, error) {
	pwd := os.Getenv("PWD")
	db, err := sql.Open("sqlite3", pwd+"/testing/db")
	if err != nil {
		return nil, err
	}
	return db, nil
}

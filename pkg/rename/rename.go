package rename

import (
	"errors"
	"fmt"

	"database/sql"

	"github.com/strang1ato/nhi/pkg/utils"
)

// Rename change name of given shell session
func Rename(db *sql.DB, session, newName string) error {
	indicator, err := utils.GetSessionIndicator(db, session)
	if err != nil {
		return err
	}

	if newName == "meta" {
		return errors.New("Session can't be named meta")
	}

	if err := checkIfNameIsUsed(db, newName); err != nil {
		return err
	}

	query := fmt.Sprintf("UPDATE `meta` SET name='%s' WHERE indicator = '%s';", newName, indicator)
	_, err = db.Exec(query)
	return err
}

func checkIfNameIsUsed(db *sql.DB, newName string) error {
	query := fmt.Sprintf("SELECT indicator FROM meta WHERE name = '%s';", newName)

	rows, err := db.Query(query)
	if err != nil {
		return err
	}

	if rows.Next() {
		return errors.New("The session name is already in use")
	}
	return nil
}

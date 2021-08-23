package rename

import (
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

	query := fmt.Sprintf("UPDATE `meta` SET name='%s' WHERE indicator = '%s';", newName, indicator)
	_, err = db.Exec(query)
	return err
}

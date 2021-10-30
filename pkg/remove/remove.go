package remove

import (
	"fmt"

	"database/sql"

	"github.com/strang1ato/nhi/pkg/utils"
)

func Remove(db *sql.DB, startEndRange string) error {
	billion := 1000000000
	sliceStartEndRange, err := utils.GetSliceStartEndRange(startEndRange, billion)
	if err != nil {
		return err
	}

	startRangeInt, endRangeInt, err := utils.GetStartAndEndRangeInt(sliceStartEndRange)
	if err != nil {
		return err
	}

	where := utils.GetWhereFromRange(sliceStartEndRange, startRangeInt, endRangeInt, billion, "meta")

	query := fmt.Sprintf("SELECT indicator FROM `meta` WHERE %s;", where)
	rows, err := db.Query(query)
	if err != nil {
		return err
	}

	var indicators []string
	for rows.Next() {
		var indicator string
		rows.Scan(&indicator)

		indicators = append(indicators, indicator)
	}

	for _, indicator := range indicators {
		query := fmt.Sprintf("DROP TABLE `%s`;", indicator)
		db.Exec(query)

		query = fmt.Sprintf("DELETE FROM `meta` WHERE indicator = '%s';", indicator)
		db.Exec(query)
	}

	return nil
}

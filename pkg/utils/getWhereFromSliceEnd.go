package utils

import (
	"fmt"
)

func GetWhereFromSliceEnd(sliceStartEndRange []string, startRangeInt, endRangeInt, billion int, table string) string {
	var where string
	if startRangeInt < billion && endRangeInt < billion {
		if startRangeInt < 0 && endRangeInt < 0 {
			where = fmt.Sprintf("rowid >= (SELECT max(rowid)+%s FROM `%s`) AND rowid < (SELECT max(rowid)+%s FROM `%s`)",
				sliceStartEndRange[0], table, sliceStartEndRange[1], table)
		} else if startRangeInt < 0 {
			where = fmt.Sprintf("rowid >= (SELECT max(rowid)+%s FROM `%s`) AND rowid <= %s",
				sliceStartEndRange[0], table, sliceStartEndRange[1])
		} else if endRangeInt < 0 {
			where = fmt.Sprintf("rowid > %s AND rowid < (SELECT max(rowid)+%s FROM `%s`)",
				sliceStartEndRange[0], sliceStartEndRange[1], table)
		} else {
			where = fmt.Sprintf("rowid > %s AND rowid <= %s",
				sliceStartEndRange[0], sliceStartEndRange[1])
		}
	} else if startRangeInt < billion {
		if startRangeInt < 0 {
			where = fmt.Sprintf("rowid >= (SELECT max(rowid)+%s FROM `%s`) AND indicator <= %s",
				sliceStartEndRange[0], table, sliceStartEndRange[1])
		} else {
			where = fmt.Sprintf("rowid > %s AND indicator <= %s",
				sliceStartEndRange[0], sliceStartEndRange[1])
		}
	} else if endRangeInt < billion {
		if endRangeInt < 0 {
			where = fmt.Sprintf("indicator >= %s AND rowid < (SELECT max(rowid)+%s FROM `%s`)",
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

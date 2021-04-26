package main

import (
	"log"

	"github.com/strang1ato/nhi/cmd"
)

func main() {
	if err := cmd.Run(); err != nil {
		log.Panicln(err)
	}
}

package main

import (
	"fmt"
	"os"

	"github.com/strang1ato/nhi/cmd"
)

func main() {
	if err := cmd.Run(); err != nil {
		fmt.Fprintln(os.Stderr, err)
	}
}

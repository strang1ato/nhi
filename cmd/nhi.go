package cmd

import (
	"errors"

	"github.com/alecthomas/kong"
	"github.com/strang1ato/nhi/pkg/log"
)

// Declare cli variable used by kong
var cli struct {
	Log struct {
		TableName string `arg optional`
	} `cmd help:"Shows command logs"`
}

// Run runs nhi
func Run() error {
	ctx := kong.Parse(&cli)
	switch ctx.Command() {
	case "log <table-name>":
		if err := log.Log(cli.Log.TableName); err != nil {
			return err
		}
	case "log":
		if err := log.Log(""); err != nil {
			return err
		}
	default:
		return errors.New("Command not found")
	}
	return nil
}

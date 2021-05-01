package cmd

import (
	"errors"

	"github.com/alecthomas/kong"
	"github.com/strang1ato/nhi/pkg/fetch"
	"github.com/strang1ato/nhi/pkg/log"
)

// Declare cli variable used by kong
var cli struct {
	Log struct {
		TableName string `arg optional`
	} `cmd help:"Show command logs"`

	Fetch struct {
		TableName string `arg required`
		Range     string `arg optional`
	} `cmd help:"Fetch shell session, optionally with given range of commands"`
}

// Run runs nhi
func Run() error {
	ctx := kong.Parse(&cli)
	switch ctx.Command() {
	case "log":
		if err := log.Log(""); err != nil {
			return err
		}
	case "log <table-name>":
		if err := log.Log(cli.Log.TableName); err != nil {
			return err
		}

	case "fetch <table-name>":
		if err := fetch.Fetch(cli.Fetch.TableName, ":"); err != nil {
			return err
		}
	case "fetch <table-name> <start:end>":
		if err := fetch.Fetch(cli.Fetch.TableName, cli.Fetch.Range); err != nil {
			return err
		}
	default:
		return errors.New("Command not found")
	}
	return nil
}

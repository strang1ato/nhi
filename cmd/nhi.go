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
		Session string `arg optional name:"session"`
	} `cmd help:"Show logs"`

	Fetch struct {
		Session     string `arg required name:"session"`
		StartEndRange string `arg optional name:"start:end"`
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
	case "log <session>":
		if err := log.Log(cli.Log.Session); err != nil {
			return err
		}

	case "fetch <session>":
		if err := fetch.Fetch(cli.Fetch.Session, ":"); err != nil {
			return err
		}
	case "fetch <session> <start:end>":
		if err := fetch.Fetch(cli.Fetch.Session, cli.Fetch.StartEndRange); err != nil {
			return err
		}
	default:
		return errors.New("Command not found")
	}
	return nil
}

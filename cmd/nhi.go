package cmd

import (
	"errors"

	"github.com/alecthomas/kong"
	"github.com/strang1ato/nhi/pkg/fetch"
	"github.com/strang1ato/nhi/pkg/log"
	"github.com/strang1ato/nhi/pkg/logSession"
	"github.com/strang1ato/nhi/pkg/rename"
	"github.com/strang1ato/nhi/pkg/utils"
)

// Declare cli variable used by kong
var cli struct {
	Log struct {
		Session   string `arg optional name:"session"`
		Directory string `short:"d" help:"Only show shell sessions where commands were executed in specified directory."`
		Before    string `short:"b" help:"Only show shell sessions created (or commands executed if session specified) before specified date and time. Date and time needs to be specified in the following format: \"%YY-%MM-%DD %HH:%MM:%SS\"."`
		After     string `short:"a" help:"Only show shell sessions created (or commands executed if session specified) after specified date and time. Date and time needs to be specified in the following format: \"%YY-%MM-%DD %HH:%MM:%SS\"."`
		Long      bool   `short:"l" help:"Use a long listing format."`
	} `cmd help:"Show logs"`

	Fetch struct {
		Session          string `arg required name:"session"`
		StartEndRange    string `arg optional name:"start:end"`
		Directory        string `short:"d" help:"Only fetch commands that were executed in specified directory."`
		Before           string `short:"b" help:"Only fetch commands that were executed before specified date and time. Date and time needs to be specified in the following format: \"%YY-%MM-%DD %HH:%MM:%SS\"."`
		After            string `short:"a" help:"Only fetch commands that were executed after specified date and time. Date and time needs to be specified in the following format: \"%YY-%MM-%DD %HH:%MM:%SS\"."`
		FetchChildShells bool   `short:"f" help:"Fetch content of shells executed within the session."`
	} `cmd help:"Fetch shell session, optionally with given range of commands."`

	Rename struct {
		Session string `arg required name:"session"`
		NewName string `arg required name:"new-name"`
	} `cmd help:"Rename shell session."`
}

// Run runs nhi
func Run() error {
	db, err := utils.OpenDb()
	if err != nil {
		return err
	}

	ctx := kong.Parse(&cli)
	switch ctx.Command() {
	case "log":
		err = log.Log(db, cli.Log.Directory, cli.Log.Before, cli.Log.After, cli.Log.Long)
	case "log <session>":
		err = logSession.LogSession(db, cli.Log.Session, cli.Log.Directory, cli.Log.Before, cli.Log.After, cli.Log.Long)
	case "fetch <session>":
		var indicator string
		indicator, err = utils.GetSessionIndicator(db, cli.Fetch.Session)
		if err == nil {
			err = fetch.Fetch(db, indicator, ":", cli.Fetch.Directory, cli.Fetch.Before, cli.Fetch.After, cli.Fetch.FetchChildShells)
		}
	case "fetch <session> <start:end>":
		var indicator string
		indicator, err = utils.GetSessionIndicator(db, cli.Fetch.Session)
		if err == nil {
			err = fetch.Fetch(db, indicator, cli.Fetch.StartEndRange, cli.Fetch.Directory, cli.Fetch.Before, cli.Fetch.After, cli.Fetch.FetchChildShells)
		}
	case "rename <session> <new-name>":
		err = rename.Rename(db, cli.Rename.Session, cli.Rename.NewName)
	default:
		err = errors.New("Command not found")
	}

	if err == nil {
		err = db.Close()
	} else {
		db.Close()
	}
	return err
}

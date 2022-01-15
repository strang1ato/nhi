<img src="doc/nhi-logo-200x200.png" align="left">

![workflow](https://github.com/strang1ato/nhi/actions/workflows/ci.yml/badge.svg)
[![Gitter chat](https://badges.gitter.im/gitterHQ/gitter.png)](https://gitter.im/nhi-project/community)
[![Tweet](https://img.shields.io/twitter/url/http/shields.io.svg?style=social)](https://twitter.com/intent/tweet?url=https://github.com/strang1ato/nhi)

`nhi` is a revolutionary tool which automatically captures all potentially useful information
about each executed command and everything around, and delivers powerful querying mechanism.

`nhi` keeps records of:
- command
- output of command
- exit status of command
- working directory at the end of command execution
- start time of command
- finish time of command
- shell prompt at the time of command execution
- (and much more in the future :smile:)

`nhi` also keeps records of information about shell session in general.

These features allow retrievement of commands executed in past and whole shell sessions,
as well as every other useful information in a convenient way.

`nhi` daemon is based on [eBPF](https://ebpf.io/) - a technology built into `linux kernel`.
Usage of `eBPF` guarantee a great performance and low overhead of the tool, because tracing is being **safely** done inside `kernel`.

`nhi` **does not** affect behaviour of any program/process (and OS in general).

Watch [the introductory video](https://www.youtube.com/watch?v=i7F3fJdYXSs) to see how `nhi` works in practice.

## Usage
For the full documentation, read [the nhi(1) man page](https://htmlpreview.github.io/?https://github.com/strang1ato/nhi/blob/main/doc/nhi.1.html).

For quick reference, use `nhi` `--help` flags.

## Requirements
`bash` or `zsh`, `x86_64` architecture, `systemd` and `linux kernel` 5.5+.
Some major distributions that ship with the `linux kernel` 5.5+:
- Debian 11
- Ubuntu 20.10+
- Fedora 32+

`xterm` based terminals are highly recommended. (If you don't know whether your terminal emulator is `xterm` based or not, it most likely is `xterm` based. `xterm` is a standard for terminal emulators.)

## Installation

### Ubuntu 21.10+
**Step 1**:
Ubuntu has oddly compiled `bash` and `zsh` binaries which are missing some data required by `nhi`. To install shells that are compiled "normally", like on every other distro run:

    sudo apt-get remove zsh-common
    sudo add-apt-repository ppa:strang1ato/default-bash-and-zsh
    sudo apt-get update
    sudo apt-get install --reinstall bash

and if you were using `zsh`:

    sudo apt-get install zsh

From now `bash` and `zsh` will be upgraded/installed only from the newly added repository.

**Step 2**: Run:

    sudo add-apt-repository ppa:strang1ato/nhi
    sudo apt-get update
    sudo apt-get install nhi

**Step 3**: Add to the end of your `.bashrc`:

    source /etc/nhi/nhi.bash

And if you use `zsh` add to the end of your `.zshrc`:

    source /etc/nhi/nhi.zsh

**Step 4**: Restart your computer.

### Other distributions
**Step 1**: Install `objdump`, `awk`, `sqlite3`, `libsqlite3-dev` and `libbpf-dev` (example for linux debian systems):

    sudo apt-get install binutils gawk sqlite3 libsqlite3-dev libbpf-dev

**Step 2**: Download all seven files from the latest [release](https://github.com/strang1ato/nhi/releases), and put them in a new empty directory.

**Step 3**: Go to the new directory and run:

    sudo -E bash ./install

**Step 4**: Restart your computer.

### Testing
In order to check if you installed `nhi` succesfully open new terminal window and execute for example:

    echo nhi test

and

    nhi fetch {-1}

If you see "nhi test" once again, it means that `nhi` is properly installed.

## Contributions
Contributions are welcome! Any tips and suggestions are appreciated. If you found any bug feel free to submit a new [issue](https://github.com/strang1ato/nhi/issues).

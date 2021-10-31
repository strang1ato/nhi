<img src="doc/nhi-logo-200x200.png" align="left">

![workflow](https://github.com/strang1ato/nhi/actions/workflows/ci.yml/badge.svg)
[![Gitter chat](https://badges.gitter.im/gitterHQ/gitter.png)](https://gitter.im/nhi-project/community)

`nhi` is a revolutionary tool which automatically captures every potentially useful information
about each executed command and everything around, and delivers powerful querying mechanism.

`nhi` keeps records of:
- output of command
- exit status of command
- working directory at the end of command execution
- start time of command
- finish time of command
- shell prompt at the time of execution
- (and much more in the future :smile:)

`nhi` also keeps record of infomations about shell session in general.

These features allow retrievement of commands executed in past and whole shell sessions,
as well as every other useful information in convenient way.

`nhi` daemon is based on [eBPF](https://ebpf.io/) - a technology built-in `linux kernel`.
Usage of `eBPF` guarantee a great performance and low overhead of the tool, because tracing is being **safely** done inside `kernel`.

`nhi` **does not** affect behaviour of any program/process (and OS in general).

## Usage
For the full documentation, read [the nhi(1) man page](https://htmlpreview.github.io/?https://github.com/strang1ato/nhi/blob/main/doc/nhi.1.html).

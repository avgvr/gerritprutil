# gerritprutil

## Table of Contents

- [Description](#Description)
- [Build](#Build)
- [License](#License)

--

## Description

The standalone background routine that handles GitHub repository pull requests
and transforms them to a Gerrit change chain.

*gerritptutil* works by the following algorithm: GitHub webhooks are used to
fetch new pull requests, against a polling. Fetched commits compose to an
internal format and pushes them to a Gerrit service under patch sender Gerrit
account. A Gerrit account is created if it has not.

The *Relation chain* of commits from the *comparing branch* (the source branch
containing new changes) is arranged as the result of the *gerritprutil* work.

In processed PR, the *gerritprutil* leaves a message about the patch was moved
to a Gerrit.

This tool is under active development, so information will be updated, as new
changes are submitted.

## Build

**Prerequisites** is the `cmake` installed on the system from the build was
performed. Use your own packet manager to acquire the `cmake`.

Generate build files and compile the source code.

```bash
cmake -B build && cmake --build build
```

Run tests if amend

```bash
ctest --test-dir build
```

## License

This project is licence under the BSD 2-Clause License - see the
[LICENSE](LICENSE) file.

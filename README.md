# TWMailer

A simple multithreaded mail server and client written in C++20.  
Features LDAP-based LOGIN authentication, per-IP blacklist after 3 failed logins, and
filesystem-based mail storage.

## Build

Requirements (Linux):
- g++ with C++20 support
- OpenLDAP dev libs (`libldap`, `liblber`)
- make

Build everything:
```bash
make -j
```

Clean:
```bash
make clean
```

## Run

Start server:
```bash
./bin/server <port> <mail-spool-directory>
# example
./bin/server 6543 mail-spool
```

Start client:
```bash
./bin/client <ip> <port>
# example
./bin/client 127.0.0.1 6543
```

## Protocol (short)

All messages are line-based (`\n`).

Commands:
- `LOGIN` + username + password
- `SEND` + receiver + subject + message body + `.`
- `LIST`
- `READ` + id
- `DEL` + id
- `QUIT`

After successful `LOGIN`, all commands except `QUIT` are allowed.

## Storage

Messages are stored under:
```
mail-spool/<user>/<id>.txt
```

Format:
```
FROM: <sender>
SUBJECT: <subject>

<body>
```

## Security

- 3 failed LOGIN attempts per IP → IP blocked for 60 seconds
- Blacklist persists in `blacklist.txt`

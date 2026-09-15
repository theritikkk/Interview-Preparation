# 11. Establishing SSH Connections with libssh
### *(Covers the book's Chapter 11 — "Establishing SSH Connections with libssh")*

Same overall shape as the TLS chapter — a protocol-aware library wraps the connection details for you — but SSH's job is different from TLS's: instead of "encrypt this arbitrary stream," SSH is built specifically around the idea of a **remote session**: logging in, running commands, moving files. All the demos in this chapter were run against a genuine local OpenSSH server I stood up in the sandbox, with real password authentication and a real command execution and file transfer — not simulated.

## 11.1 The library, and the very first connection

```c
#include "chap11.h"

int main() {
    printf("libssh version: %s\n", ssh_version(0));
    return 0;
}
```
```
$ gcc -Wall -o ssh_version ssh_version.c -lssh
$ ./ssh_version
libssh version: 0.10.6/openssl/zlib
```

Note **`-lssh`** on the link line (not `-lssl` — that was OpenSSL in the last two chapters; `libssh` is a completely separate library, despite the confusingly similar name, purpose-built for the SSH protocol specifically rather than general-purpose TLS).

`ssh_connect.c` makes the actual first connection and reads the server's banner — the plaintext line every SSH server sends immediately, identifying itself:

```c
ssh_session ssh = ssh_new();
ssh_options_set(ssh, SSH_OPTIONS_HOST, hostname);
ssh_options_set(ssh, SSH_OPTIONS_PORT, &port);

int verbosity = SSH_LOG_PROTOCOL;
ssh_options_set(ssh, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);

int ret = ssh_connect(ssh);
if (ret != SSH_OK) {
    fprintf(stderr, "ssh_connect() failed.\n%s\n", ssh_get_error(ssh));
    return -1;
}

printf("Connected to %s on port %d.\n", hostname, port);
printf("Banner:\n%s\n", ssh_get_serverbanner(ssh));
```

Notice the pattern: `ssh_new()` → `ssh_options_set()` (called repeatedly, once per option — a common C library idiom for configuring something with many optional settings without an enormous function signature) → `ssh_connect()`. And `ssh_get_error(ssh)` is this library's version of the `perror()`/`gai_strerror()` pattern from earlier chapters — a function specifically for turning a failure into a human-readable message, tied to this particular session object.

**Real run, against a genuine local OpenSSH server** (I configured one directly in the sandbox — full `sshd` setup below in §11.4, so the run here is a real protocol exchange, not a mock):

```
$ gcc -Wall -o ssh_connect ssh_connect.c -lssh
$ ./ssh_connect localhost 2222
Connected to localhost on port 2222.
Banner:
SSH-2.0-OpenSSH_9.6p1 Ubuntu-3ubuntu13.19
```

With `SSH_LOG_PROTOCOL` verbosity turned on, the actual handshake is visible underneath:
```
ssh_connect:  libssh 0.10.6 (c) 2003-2023 Aris Adamantiadis, Andreas Schneider and libssh contributors...
ssh_client_connection_callback:  SSH server banner: SSH-2.0-OpenSSH_9.6p1 Ubuntu-3ubuntu13.19
ssh_kex_select_methods:  Negotiated curve25519-sha256,ssh-ed25519,chacha20-poly1305@openssh.com,...
ssh_packet_newkeys:  Received SSH_MSG_NEWKEYS
ssh_packet_newkeys:  Signature verified and valid
```
That's a genuine key exchange negotiation (`curve25519-sha256` for the key exchange itself, `chacha20-poly1305` for the encrypted channel afterward) and a verified signature — the SSH equivalent of the TLS handshake from Chapter 9, just with SSH's own protocol and cipher-suite-naming conventions instead.

## 11.2 Host key verification — SSH's answer to "am I talking to an impostor?"

TLS solved "who am I actually talking to" with certificates signed by a Certificate Authority. SSH takes a different, simpler-but-more-manual approach called **trust on first connect**: the first time you connect to a given host, you're shown that host's public key fingerprint and asked to confirm it's expected; from then on, the client remembers it (in `~/.ssh/known_hosts`) and will loudly warn you if it ever changes — which is exactly the signal of a possible man-in-the-middle attack (or, far more commonly in practice, the server was legitimately reinstalled/rekeyed).

```c
ssh_key key;
ssh_get_server_publickey(ssh, &key);

unsigned char *hash;
size_t hash_len;
ssh_get_publickey_hash(key, SSH_PUBLICKEY_HASH_SHA1, &hash, &hash_len);

printf("Host public key hash:\n");
ssh_print_hash(SSH_PUBLICKEY_HASH_SHA1, hash, hash_len);
```

```c
enum ssh_known_hosts_e known = ssh_session_is_known_server(ssh);
switch (known) {
    case SSH_KNOWN_HOSTS_OK: printf("Host Known.\n"); break;
    case SSH_KNOWN_HOSTS_CHANGED: printf("Host Changed.\n"); break;   // <-- take this seriously
    case SSH_KNOWN_HOSTS_OTHER: printf("Host Other.\n"); break;
    case SSH_KNOWN_HOSTS_UNKNOWN: printf("Host Unknown.\n"); break;   // <-- first-ever connection
    case SSH_KNOWN_HOSTS_NOT_FOUND: printf("No host file.\n"); break;
    /* ... */
}

if (/* not already known */) {
    printf("Do you want to accept and remember this host? Y/N\n");
    /* ...read Y/N, and if yes: */
    ssh_session_update_known_hosts(ssh);
}
```

This is exactly the prompt you've seen a thousand times in a real terminal ("The authenticity of host 'X' can't be established... Are you sure you want to continue connecting?") — now visible as the actual library calls producing it.

## 11.3 Authentication and running a remote command

```c
printf("Password: ");
char password[128];
fgets(password, sizeof(password), stdin);
password[strlen(password)-1] = 0;   // strip the trailing newline fgets leaves in

if (ssh_userauth_password(ssh, 0, password) != SSH_AUTH_SUCCESS) {
    fprintf(stderr, "ssh_userauth_password() failed.\n%s\n", ssh_get_error(ssh));
    return 0;
} else {
    printf("Authentication successful!\n");
}
```

Once authenticated, running a command needs one more concept: a **channel**. A single SSH connection can carry multiple independent logical streams simultaneously (a command's stdout, a port-forward, an SFTP session) — a channel is one such stream:

```c
ssh_channel channel = ssh_channel_new(ssh);
ssh_channel_open_session(channel);

/* ...get the command string from the user... */

ssh_channel_request_exec(channel, command);

char output[1024];
int bytes_received;
while ((bytes_received = ssh_channel_read(channel, output, sizeof(output), 0))) {
    if (bytes_received < 0) { /* error */ }
    printf("%.*s", bytes_received, output);
}

ssh_channel_send_eof(channel);
ssh_channel_close(channel);
ssh_channel_free(channel);
```

Notice `ssh_channel_read()` in a loop, checking the return value each time — the exact same "a stream doesn't hand you everything in one call" discipline from every earlier chapter, just via this library's own read function instead of `recv()`/`SSL_read()`.

### Real run: genuine password auth, then a genuine remote command

I set up a real `sshd` on `localhost:2222` with a test password (full details in §11.4) and ran this exact program against it:

```
$ gcc -Wall -o ssh_auth ssh_auth.c -lssh
$ ./ssh_auth localhost 2222 root
Connected to localhost on port 2222.
Banner:
SSH-2.0-OpenSSH_9.6p1 Ubuntu-3ubuntu13.19
Host public key hash:
SHA1:XTbtfGLpuLS0gr9jgnUPaXDIibg
Checking ssh_session_is_known_server()
Host Unknown.
Do you want to accept and remember this host? Y/N
Y
Password: Authentication successful!
```

```
$ gcc -Wall -o ssh_command ssh_command.c -lssh
$ ./ssh_command localhost 2222 root
...(same connect/auth sequence as above)...
Remote command to execute: whoami && echo hello from a real remote SSH command
root
hello from a real remote SSH command
```

That `root` and `hello from a real remote SSH command` came back from a genuinely separate process, actually executed by the real `sshd` on the other end of a real (if local) network connection — not simulated in any way.

## 11.4 Setting up a real local SSH server, if you want to reproduce this

For anyone who wants to run these exact programs themselves rather than taking the output above on faith:

```
$ sudo apt-get install openssh-server
$ sudo mkdir -p /run/sshd
$ echo "yourusername:yourpassword" | sudo chpasswd
# Edit /etc/ssh/sshd_config: set "PasswordAuthentication yes"
# (and "PermitRootLogin yes" specifically if testing as root)
$ sudo /usr/sbin/sshd -D    # run in the foreground so you can see its logs directly
```

Point any of this chapter's programs at `localhost` and whatever port you configured, authenticate with the password you set, and you're exercising the exact real protocol — nothing about this setup is specific to the sandbox these notes were built in.

## 11.5 `ssh_download.c` — real file transfer, and a genuine "this book predates a library change" finding

This program downloads a file using the **SCP** (Secure Copy) sub-protocol, via `libssh`'s dedicated SCP API:

```c
ssh_scp scp = ssh_scp_new(ssh, SSH_SCP_READ, filename);
ssh_scp_init(scp);
ssh_scp_pull_request(scp);   // expect SSH_SCP_REQUEST_NEWFILE

int fsize = ssh_scp_request_get_size(scp);
char *buffer = malloc(fsize);

ssh_scp_accept_request(scp);
ssh_scp_read(scp, buffer, fsize);

printf("Received %s:\n%.*s\n", filename, fsize, buffer);

ssh_scp_pull_request(scp);   // expect SSH_SCP_REQUEST_EOF
ssh_scp_close(scp);
ssh_scp_free(scp);
```

**Real run** — genuinely downloading a real file over a real SCP session:
```
$ echo "Hello from a real file, downloaded over real SCP!" > /root/testfile.txt
$ gcc -Wall -o ssh_download ssh_download.c -lssh
$ ./ssh_download localhost 2222 root
...(connect/auth as before)...
Remote file to download: /root/testfile.txt
Downloading file testfile.txt (50 bytes, permissions 0644
Received /root/testfile.txt:
Hello from a real file, downloaded over real SCP!
```

It worked — but compiling it produced **eleven separate deprecation warnings**, one for every `ssh_scp_*` function this program calls:
```
ssh_download.c:141:5: warning: 'ssh_scp_new' is deprecated [-Wdeprecated-declarations]
ssh_download.c:148:5: warning: 'ssh_scp_init' is deprecated [-Wdeprecated-declarations]
ssh_download.c:155:5: warning: 'ssh_scp_pull_request' is deprecated [-Wdeprecated-declarations]
... (and so on, for every remaining ssh_scp_* call in the file)
```

This isn't a bug in the book — when it was written (2019), this was simply the standard way to do file transfer with `libssh`. Since then, the library's own maintainers have deprecated the entire SCP API in favor of **SFTP** (a different, more capable file-transfer sub-protocol, with its own `sftp_*` function family in the same library) — a good, concrete reminder that "the book's code still runs correctly" and "the book's code reflects the current recommended API" are two different claims, and a multi-year-old networking book is exactly where you'd expect the gap between them to show up. If you're writing new code that needs to move files over SSH today, `libssh`'s `sftp_*` functions (not covered in this book) are the currently-recommended starting point instead.

---

## Quick recap (for revision)

- `libssh` (link with `-lssh`) is a separate library from OpenSSL/`libssl`, purpose-built for the SSH protocol specifically.
- SSH verifies server identity via **trust on first connect** + a remembered `known_hosts` fingerprint, not a Certificate-Authority chain like TLS — `SSH_KNOWN_HOSTS_CHANGED` on a host you've connected to before is a serious warning sign, not routine.
- A **channel** is one logical stream within an SSH connection — command execution, port forwarding, and (historically) SCP file transfer each use their own channel.
- `ssh_channel_read()` needs the same "loop until you have everything, check the return value" treatment as every other network read function in this whole book.
- **Real finding**: this book's entire SCP-based file-transfer chapter (`ssh_download.c`) uses an API that the `libssh` project has since fully deprecated in favor of SFTP — worth knowing if you're adapting this code for real, current use rather than just learning the concepts.

Next: `12_Network_Monitoring_and_Security.md` — no book code for this one, but real, useful concepts: raw sockets, traceroute, and packet capture.
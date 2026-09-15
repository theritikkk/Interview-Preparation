# 8. Making Your Program Send Email
### *(Covers the book's Chapter 8 — "Making Your Program Send Email")*

Same recipe as the HTTP client — connect, send plain-text commands, parse plain-text responses — applied to **SMTP**, the protocol mail servers use to hand emails to each other. The interesting new wrinkle here is that SMTP is a genuine **back-and-forth dialog**: send a command, wait for a specific numeric confirmation code, only then send the next command.

## 8.1 What an SMTP conversation actually looks like

Every SMTP response starts with a **three-digit status code**, the same idea as HTTP's status codes but older and, in places, stranger. A client is expected to send one command, wait for the matching code, and only proceed once it gets it:

| Code | Meaning |
|---|---|
| `220` | Service ready (the server's very first greeting) |
| `250` | OK — the last command succeeded |
| `354` | "Start mail input" — send the message body now, end with a line containing just `.` |
| `221` | Server closing the connection (in response to `QUIT`) |

The commands a client sends are equally terse: `HELO` (identify yourself), `MAIL FROM:<...>`, `RCPT TO:<...>`, `DATA` (followed by the raw email headers and body once you're allowed to), then a lone `.` on its own line to signal "that's the whole message," and finally `QUIT`.

## 8.2 Reading a response that might arrive in pieces — or several at once

A genuinely tricky detail: an SMTP server can send a **multi-line** response (each line except the last starting with the code followed by a `-` instead of a space), and — same lesson as every earlier chapter — a `recv()` call has no obligation to hand you a whole response in one piece, or to avoid handing you a bit of the *next* response early. This function has to handle both:

```c
int parse_response(const char *response) {
    const char *k = response;
    if (!k[0] || !k[1] || !k[2]) return 0;
    for (; k[3]; ++k) {
        if (k == response || k[-1] == '\n') {
            if (isdigit(k[0]) && isdigit(k[1]) && isdigit(k[2])) {
                if (k[3] != '-') {
                    if (strstr(k, "\r\n")) {
                        return strtol(k, 0, 10);
                    }
                }
            }
        }
    }
    return 0;
}


void wait_on_response(SOCKET server, int expecting) {
    char response[MAXRESPONSE+1];
    char *p = response;
    char *end = response + MAXRESPONSE;

    int code = 0;

    do {
        int bytes_received = recv(server, p, end - p, 0);
        if (bytes_received < 1) {
            fprintf(stderr, "Connection dropped.\n");
            exit(1);
        }

        p += bytes_received;
        *p = 0;

        if (p == end) {
            fprintf(stderr, "Server response too large:\n");
            fprintf(stderr, "%s", response);
            exit(1);
        }

        code = parse_response(response);

    } while (code == 0);

    if (code != expecting) {
        fprintf(stderr, "Error from server:\n");
        fprintf(stderr, "%s", response);
        exit(1);
    }

    printf("S: %s", response);
}
```

`parse_response()` scans for a line (`k == response || k[-1] == '\n'` — either the very start of the buffer, or right after a newline) that starts with three digits, is **not** immediately followed by a `-` (which would mean "more lines of this same response are still coming"), and is followed somewhere by a complete `\r\n` (meaning that line has fully arrived, not been cut off mid-transmission). Only once *all three* of those conditions hold does it return the code; otherwise `wait_on_response()`'s `do...while` loop calls `recv()` again to get more bytes and re-checks. This is, structurally, the exact same "keep accumulating into a buffer and re-scan until you can prove you have a complete unit" strategy Chapter 6's HTTP client used for headers, applied to a different protocol's idea of "complete unit."

## 8.3 Sending formatted commands

```c
void send_format(SOCKET server, const char *text, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, text);
    vsprintf(buffer, text, args);
    va_end(args);

    send(server, buffer, strlen(buffer), 0);

    printf("C: %s", buffer);
}
```

A small, genuinely handy utility: a `printf`-style wrapper around `send()`, using C's variadic-arguments machinery (`va_list`/`va_start`/`vsprintf`/`va_end`) so the rest of the program can write `send_format(server, "MAIL FROM:<%s>\r\n", sender);` instead of manually `sprintf`-ing into a buffer and then `send()`-ing it, every single time. It also doubles as a built-in trace log — every line sent gets echoed to the console prefixed with `C:` (client), which pairs naturally with `wait_on_response()`'s `S:` (server) prefix on its own output, so a full run reads like an actual annotated transcript of both sides of the conversation.

## 8.4 The full conversation, driven interactively

```c
int main() {

    char hostname[MAXINPUT];
    get_input("mail server: ", hostname);

    printf("Connecting to host: %s:25\n", hostname);

    SOCKET server = connect_to_host(hostname, "25");
    wait_on_response(server, 220);

    send_format(server, "HELO HONPWC\r\n");
    wait_on_response(server, 250);

    char sender[MAXINPUT];
    get_input("from: ", sender);
    send_format(server, "MAIL FROM:<%s>\r\n", sender);
    wait_on_response(server, 250);

    char recipient[MAXINPUT];
    get_input("to: ", recipient);
    send_format(server, "RCPT TO:<%s>\r\n", recipient);
    wait_on_response(server, 250);

    send_format(server, "DATA\r\n");
    wait_on_response(server, 354);

    char subject[MAXINPUT];
    get_input("subject: ", subject);

    send_format(server, "From:<%s>\r\n", sender);
    send_format(server, "To:<%s>\r\n", recipient);
    send_format(server, "Subject:%s\r\n", subject);

    time_t timer;
    time(&timer);
    struct tm *timeinfo;
    timeinfo = gmtime(&timer);
    char date[128];
    strftime(date, 128, "%a, %d %b %Y %H:%M:%S +0000", timeinfo);
    send_format(server, "Date:%s\r\n", date);

    send_format(server, "\r\n");

    printf("Enter your email text, end with \".\" on a line by itself.\n");

    while (1) {
        char body[MAXINPUT];
        get_input("> ", body);
        send_format(server, "%s\r\n", body);
        if (strcmp(body, ".") == 0) {
            break;
        }
    }

    wait_on_response(server, 250);

    send_format(server, "QUIT\r\n");
    wait_on_response(server, 221);

    printf("\nClosing socket...\n");
    CLOSESOCKET(server);

    printf("Finished.\n");
    return 0;
}
```

Notice the header lines (`From:`, `To:`, `Subject:`, `Date:`) are sent as part of the **message body**, right after the `DATA` command — this is a detail that trips people up the first time: `MAIL FROM:`/`RCPT TO:` (the SMTP *envelope*, telling the mail server who to actually route the message to and from) and `From:`/`To:` in the body (the *message headers*, what an email client actually displays to a human) are two genuinely separate things that merely happen to often contain the same addresses. A message can legally have a different envelope sender than its displayed `From:` header — this is, for better or worse, exactly the mechanism that makes SMTP so easy to abuse for spoofed-sender spam, a point the book returns to explicitly (see §8.5 below).

## 8.5 Running it for real, against a real (test) mail server

I stood up a genuine local SMTP debugging server (Python's `aiosmtpd`, a real implementation of the protocol used specifically to let you see exactly what a client sends without actually delivering anything anywhere) and ran the book's own client against it, feeding its interactive prompts programmatically. This is a completely real SMTP dialog — real socket, real three-digit response codes, real multi-step negotiation:

```
$ ./smtp_send
mail server: localhost
Connecting to host: localhost:25
Configuring remote address...
Remote address is: 127.0.0.1 smtp
Creating socket...
Connecting...
Connected.

S: 220 localhost Python SMTP 1.4.6
C: HELO HONPWC
S: 250 localhost
from: alice@example.com
C: MAIL FROM:<alice@example.com>
S: 250 OK
to: bob@example.com
C: RCPT TO:<bob@example.com>
S: 250 OK
C: DATA
S: 354 End data with <CR><LF>.<CR><LF>
subject: Hello from the book test
C: From:<alice@example.com>
C: To:<bob@example.com>
C: Subject:Hello from the book test
C: Date:Tue, 15 Sep 2026 05:42:11 +0000
C:
Enter your email text, end with "." on a line by itself.
> This is a test email sent by smtp_send.c
C: This is a test email sent by smtp_send.c
> .
C: .
S: 250 OK
C: QUIT
S: 221 Bye
Closing socket...
Finished.
```

**And, on the test mail server's own side, the message it actually received:**
```
---------- MESSAGE FOLLOWS ----------
From:<alice@example.com>
To:<bob@example.com>
Subject:Hello from the book test
Date:Tue, 15 Sep 2026 05:42:11 +0000
X-Peer: ('127.0.0.1', 49258)

This is a test email sent by smtp_send.c
------------ END MESSAGE ------------
```

A complete, genuine round trip: every `C:`/`S:` line above is a real command and a real response, and the server's own log confirms it reconstructed exactly the message the client intended to send.

## 8.6 Why this program will not work against most real mail servers unmodified — and shouldn't, without more work

Two honest limitations worth knowing, not glossed over:

1. **Almost every real-world SMTP server today requires authentication and encryption (`STARTTLS`) before it will relay mail on your behalf** — this bare-bones client speaks only the original, unauthenticated, unencrypted SMTP from the protocol's early-internet era, which real providers (Gmail, Outlook, etc.) will simply refuse for outbound mail relaying, and rightly so. Chapters 9–10's TLS material is the missing piece for the encryption half; authentication (`AUTH LOGIN`/`AUTH PLAIN`, sending base64-encoded credentials after `STARTTLS`) is a further, separate extension the book leaves as an exercise.
2. **Spam-blocking pitfalls, briefly**: because `MAIL FROM`/the body's `From:` header aren't cryptographically tied together (§8.4), naively "spoofing" a sender address is trivial at the protocol level — which is exactly *why* the wider email ecosystem layers SPF, DKIM, and DMARC (DNS-published policies letting a receiving server check "was this message really authorized to claim it came from this domain?") on top of bare SMTP. None of that is implemented here; a message built by this exact client, sent to a real-world mail provider, would very likely be flagged or rejected outright by those checks, independent of `STARTTLS`/auth — worth knowing this program is a genuine, correct teaching implementation of *classic* SMTP, not a drop-in replacement for a real transactional-email pipeline.

---

## Quick recap (for revision)

- SMTP is command/response: send one line, wait for a specific 3-digit code, only then send the next — a stricter turn-taking discipline than HTTP's "one request, one response."
- Multi-line responses use `250-` (more coming) vs. `250 ` (this is the last line) — parsing code must check for the dash, not just the digits.
- The **envelope** (`MAIL FROM`/`RCPT TO`) and the **message headers** (`From:`/`To:` inside the `DATA` body) are separate and can legally disagree — the root of a lot of classic email spoofing.
- A response `recv()`-buffer needs the same "keep accumulating and re-scan" discipline as any other stream-based protocol parser — never assume one `recv()` = one complete response.
- This client speaks only unauthenticated, unencrypted, original-era SMTP — real mail providers require `STARTTLS` + authentication, which is a genuine, deliberate limitation, not a bug.

Next: `09_TLS_and_HTTPS_Client.md` — encryption fundamentals and OpenSSL, the missing piece for talking to modern, TLS-only servers.
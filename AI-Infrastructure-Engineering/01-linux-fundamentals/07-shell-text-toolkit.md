# Shell Text Toolkit for Infrastructure Work

> **Why this matters:** in an incident and in an OA, most evidence is text — logs, `ss` output, `ps`, JSON, CSV. The engineer who can extract the answer in one pipeline is faster and more accurate than the one who scrolls.

This note is about **reading and transforming text**, not about writing large programs. Bash scripting is in [Phase 7](../07-automation/01-bash-for-operations.md).

---

## 1. Quoting and expansion (the source of most bugs)

| Form | Behaviour | Example |
|---|---|---|
| `'single'` | Literal; no expansion | `echo '$HOME'` → `$HOME` |
| `"double"` | Expands variables/command substitution | `echo "$HOME"` → `/home/user` |
| `$'...'` | ANSI-C escapes | `$'a\tb'` |
| Backslash | Escapes one character | `echo \$HOME` |
| `$(cmd)` | Command substitution (preferred) | `now=$(date -Is)` |
| `` `cmd` `` | Legacy substitution; avoid nesting | `` `date` `` |
| `"$var"` | Always quote unless you want word splitting | `rm -- "$file"` |
| `"${arr[@]}"` | Expand array elements safely | `for x in "${arr[@]}"; do ...` |
| `*`, `?`, `[abc]` | Pathname globbing by the shell | `ls /var/log/*.log` |

```bash
$ name="web 01"
$ echo $name
web 01
$ echo "$name"
web 01
$ touch $name          # creates two files: "web" and "01"
$ touch "$name"        # creates one file: "web 01"
```

**Rule:** quote variables in commands. Unquoted variables undergo word splitting and globbing, which is how filenames with spaces become two arguments.

```bash
# safe loop over find results
find /var/log -type f -name '*.log' -print0 |
while IFS= read -r -d '' f; do
  printf '%s\n' "$f"
done
```

`-print0` + `read -d ''` handles spaces, newlines, and special characters. `for f in $(find ...)` does not.

---

## 2. Pipes, redirection, exit codes

```bash
cmd > file          # stdout to file (truncate)
cmd >> file         # stdout append
cmd 2> err          # stderr to file
cmd > out 2>&1      # stdout + stderr to out
cmd &> out          # bash shorthand for both
cmd | tee out       # stdout to screen and file
cmd <<'EOF'         # here-doc, no expansion
...
EOF
cmd <<< "string"    # here-string
diff <(cmd1) <(cmd2)  # process substitution
```

Every command returns an exit status `0–255`; `0` means success.

```bash
$ grep -q root /etc/passwd; echo $?
0
$ grep -q nosuchuser /etc/passwd; echo $?
1
$ ls /does/not/exist 2>/dev/null; echo $?
2
```

In a pipeline, the exit status is the **last** command by default:

```bash
$ false | true; echo $?
0
$ set -o pipefail
$ false | true; echo $?
1
```

Use `set -o pipefail` in scripts. `grep` returns `1` for "no match", which is not the same as an error; account for it (`if grep -q ...; then`).

---

## 3. `grep`: find lines

```bash
grep 'ERROR' app.log
grep -i 'error' app.log              # case-insensitive
grep -v 'DEBUG' app.log              # invert
grep -n 'ERROR' app.log              # line numbers
grep -c 'ERROR' app.log              # count
grep -r 'listen 8080' /etc/nginx     # recursive
grep -E 'ERROR|WARN' app.log         # extended regex
grep -F 'a.b.c' file                 # fixed string, no regex
grep -l 'pattern' *.log              # files with matches
grep -A3 -B3 'panic' app.log         # context
grep -o 'user=[^ ]*' app.log         # print only match
grep -P '(?<=user=)\w+' app.log      # PCRE (GNU grep)
```

```bash
$ grep -n 'ERROR' /var/log/app.log | head
42:2026-03-15T10:22:03Z ERROR upstream timeout service=api upstream=10.0.0.5:8080
57:2026-03-15T10:22:11Z ERROR upstream timeout service=api upstream=10.0.0.5:8080
```

`grep` is line-oriented. For fields, move to `awk` or `cut`.

---

## 4. `sed`: stream edit

```bash
sed 's/foo/bar/' file               # first occurrence per line
sed 's/foo/bar/g' file              # all occurrences
sed -i.bak 's/foo/bar/g' file       # in-place, keep .bak
sed -n '10,20p' file                # print lines 10–20
sed -n '/START/,/END/p' file        # print a range
sed '/^#/d; /^$/d' file             # delete comments and blanks
sed -E 's/([0-9]+)ms/\1 milliseconds/' file
sed 's/^/PREFIX /' file
```

Examples:

```bash
# remove comments and blank lines from a config for review
sed '/^\s*#/d; /^\s*$/d' /etc/ssh/sshd_config

# replace a listen port
sed -i 's/listen 80;/listen 8080;/' /etc/nginx/sites-enabled/default
```

**Safety:** `sed -i` rewrites the file. On a symlink, it may replace the link with a regular file (GNU sed has `--follow-symlinks`). Back up first (`-i.bak`) and verify with `diff`.

---

## 5. `awk`: fields, patterns, aggregation

`awk` is a small programming language. For operations, learn four things: fields, patterns, `BEGIN/END`, and arrays.

```bash
awk '{print $1}' file                       # first whitespace-separated field
awk -F: '{print $1, $3}' /etc/passwd        # colon-separated
awk '$3 > 100 {print $1, $3}' data.txt      # condition
awk 'NR==1 {print}' file                    # header
awk 'NR>1 {sum += $2} END {print sum}' file # sum
awk '{count[$1]++} END {for (k in count) print count[k], k}' file
```

Default field separator is whitespace (spaces/tabs). `-F:` sets `:`. `$0` is the whole line, `NF` the number of fields, `NR` the record number.

### Top talkers from an access log

```bash
$ awk '{print $1}' /var/log/nginx/access.log | sort | uniq -c | sort -nr | head
   9120 10.0.1.22
   4210 10.0.2.15
    980 203.0.113.7
```

### HTTP status distribution

```bash
$ awk '{count[$9]++} END {for (s in count) print s, count[s]}' /var/log/nginx/access.log | sort -k2 -nr
200 18422
404 410
502 122
504 31
```

### Slow requests (assuming `$request_time` is the last field)

```bash
$ awk '$NF > 2 {print $NF, $7}' /var/log/nginx/access.log | sort -nr | head
8.421 /api/report
5.112 /api/export
2.904 /api/search
```

### Failed SSH sources

```bash
$ grep 'Failed password' /var/log/auth.log |
  awk '{for (i=1;i<=NF;i++) if ($i=="from") print $(i+1)}' |
  sort | uniq -c | sort -nr | head
  142 203.0.113.7
   88 198.51.100.4
```

`awk` handles numbers as doubles; for exact large integers or currency, use a language with decimal types. For logs, it is fine.

---

## 6. `find`: locate by metadata

```bash
find /var/log -type f -name '*.log'
find / -xdev -type f -size +100M -printf '%s %p\n' 2>/dev/null | sort -nr | head
find /var/log -type f -mtime +7
find /var/log -type f -mmin -30
find /path -user app -group app
find /path -perm 0644
find /path -perm -4000 -type f          # SUID
find /path -type f -name '*.tmp' -delete
find /path -type f -exec chmod 640 {} +
find /path -type f -print0 | xargs -0 -r md5sum
```

| Predicate | Meaning |
|---|---|
| `-type f/d/l` | File, directory, symlink |
| `-name`, `-iname` | Glob on basename (case-sensitive/insensitive) |
| `-path` | Glob on full path |
| `-size +100M` | Larger than 100 MiB (`c` bytes, `k`, `M`, `G`) |
| `-mtime +7` | Modified more than 7 days ago |
| `-mmin -30` | Modified within 30 minutes |
| `-user`, `-group` | Owner/group |
| `-perm -4000` | All bits in mask set (SUID) |
| `-maxdepth N` | Limit recursion |
| `-xdev` | Stay on one filesystem |

Actions:

```bash
-print                 # default
-printf '%s %p\n'      # size and path
-exec cmd {} \;        # one command per file
-exec cmd {} +         # batch arguments (faster)
-delete                # delete matches (dangerous)
```

**Safety:** run `find ... -print` first, then replace `-print` with `-delete` or `-exec`. `-delete` implies `-depth` and will remove directories if they match; be precise.

### `xargs`

```bash
find /var/log -name '*.log' -print0 | xargs -0 -r grep -l 'ERROR'
printf '%s\n' a b c | xargs -n1 echo
```

`-0` reads NUL-separated input (pair with `find -print0`). `-r` avoids running the command with no input (GNU). Without `-0`, spaces break arguments.

---

## 7. `sort`, `uniq`, `cut`, `tr`

```bash
sort file
sort -n file              # numeric
sort -h file              # human sizes (1K, 2M)
sort -k2,2 -n file        # by field 2
sort -t: -k3,3n /etc/passwd
sort -r file              # reverse
sort -u file              # unique
uniq -c                   # count adjacent duplicates (input must be sorted)
uniq -d                   # only duplicates
cut -d: -f1,3 /etc/passwd
cut -c1-20 file
tr '[:upper:]' '[:lower:]' < file
tr -s ' ' < file          # squeeze repeated spaces
tr -d '\r' < file         # remove CR (Windows files)
```

Classic pipeline:

```bash
awk '{print $1}' access.log | sort | uniq -c | sort -nr | head
```

`uniq` only collapses **adjacent** duplicates. Forgetting `sort` is the most common mistake.

`sort -h` understands `1K`, `2M`, `3G` — useful for `du -h` output.

---

## 8. `jq` for JSON

```bash
jq '.' file.json
jq -r '.items[].metadata.name' file.json
jq -r '.items[] | select(.status.phase=="Running") | .metadata.name'
jq -r '.[] | [.name, .status] | @tsv'
jq 'keys' file.json
jq '.spec.replicas' deploy.json
jq --arg ns prod '.items[] | select(.metadata.namespace==$ns)' -
```

Examples:

```bash
# Pod names that are not Running
kubectl get pods -o json | jq -r '.items[] | select(.status.phase!="Running") | .metadata.name'

# Image and restart count
kubectl get pods -o json | jq -r '.items[] | [.metadata.name, .spec.containers[0].image, (.status.containerStatuses[0].restartCount|tostring)] | @tsv'

# Error lines from JSON logs
jq -r 'select(.level=="error") | [.ts, .service, .msg] | @tsv' app.jsonl | tail -20
```

`jq -r` removes quotes for shell-friendly output. `@tsv`/`@csv` format arrays. `select()` filters. `//` provides a default: `.value // "n/a"`.

---

## 9. Operational recipes

### Top processes by memory (PSS not available, RSS is a start)

```bash
ps -eo pid,user,rss,pmem,cmd --sort=-rss | head
```

### Listeners and owning process

```bash
ss -tulpn | awk 'NR>1 {print $1, $5, $7}'
```

### Established connections by remote IP

```bash
ss -tn state established | awk 'NR>1 {split($5,a,":"); print a[1]}' | sort | uniq -c | sort -nr | head
```

### Logs with a time window

```bash
journalctl -u app --since "2026-03-15 10:00" --until "2026-03-15 10:30" -o short-iso
```

### Count 5xx by upstream

```bash
awk '$9 ~ /^5/ {print $9, $NF}' access.log | sort | uniq -c | sort -nr
```

### Find large files on one filesystem

```bash
find / -xdev -type f -size +500M -printf '%s %p\n' 2>/dev/null | sort -nr | head
```

### Top directories by inode count

```bash
find /var -xdev -type f -printf '%h\n' | sort | uniq -c | sort -nr | head
```

### CSV to columns

```bash
awk -F, 'NR==1{print; next} $3>100 {print}' report.csv | column -t -s,
```

---

## 10. Common mistakes

| Mistake | Why it is wrong | Better |
|---|---|---|
| Unquoted `$var` | Word splitting/globbing | `"$var"` |
| `for f in $(find ...)` | Breaks on spaces/newlines | `find -print0` + `read -d ''` |
| `uniq` without `sort` | Only adjacent lines collapse | `sort | uniq -c` |
| `grep` for structured fields | Line-oriented; brittle | `awk`/`jq` |
| `sed -i` without backup | Irreversible edit | `-i.bak`, `diff` |
| `find -delete` before `-print` | Deletes the wrong set | Dry run first |
| `xargs` without `-0` | Breaks on spaces | `-print0 | xargs -0` |
| Parsing `ls` output | Locale/space issues | `find -printf`, `stat` |
| Assuming `sort` is numeric | Lexicographic by default | `sort -n`/`-h` |
| Ignoring `grep` exit 1 | "No match" is not an error | `if grep -q` |

---

## 11. Drills

Use a lab log file or generate one. Write the pipeline before checking the answer.

1. Count requests per HTTP status code.
2. List the top 10 client IPs by request count.
3. Print all 5xx requests with their paths.
4. Find the 20 slowest requests (last field is seconds).
5. Extract all unique User-Agent strings and count them.
6. Find lines containing `ERROR` but not `DEBUG`.
7. Print lines 100–120 of a file.
8. Replace all occurrences of `10.0.0.5` with `10.0.0.6` in a copy, keeping a backup.
9. Find files larger than 100 MB under `/var`, sorted by size.
10. Find files modified in the last 30 minutes under `/etc`.
11. Count failed SSH logins by source IP.
12. Parse a JSON array and print `name` and `status` as TSV.
13. Select JSON objects where `level == "error"` and print the message.
14. Sum the third column of a CSV where the second column is `prod`.
15. Remove duplicate lines while preserving the first occurrence (`awk '!seen[$0]++'`).
16. Print the number of open sockets per state from `ss -tan`.
17. Find the PID listening on port 8080 using `ss` and `awk`.
18. Print the top 5 directories by size under `/var` (`du -xhd1`).
19. Strip carriage returns from a file edited on Windows.
20. Count how many times each pod restarted, from `kubectl get pods -o json`.

If you can do 15/20 without looking up syntax, you are ready for the command-output section of an OA.

---

## 12. Checkpoint

1. Why does `for f in $(find . -name '*.log')` fail on a filename with a space?
2. What does `set -o pipefail` change?
3. When does `grep` return exit code 1, and why is that not always an error?
4. What is the difference between `sed 's/x/y/'` and `sed 's/x/y/g'`?
5. What does `awk -F: '{print $1}'` do, and what is `$0`?
6. Why must `uniq` input be sorted?
7. What does `find -print0 | xargs -0` solve?
8. How do you select JSON objects with `jq` and print a field without quotes?
9. How do you find files larger than 1 GB on the root filesystem only?
10. What is the safest way to test a `find ... -delete` command?

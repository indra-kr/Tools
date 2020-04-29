# unixPw

### [unixPw.c](https://github.com/indra-kr/Tools/blob/master/unixPw/unixPw.c)

A tool for \*nix system password

### Compile
```sh
$ cc -o unixPw unixPw.c -lcrypt
```

### Generating a encrypted password
```sh
$ ./unixPw
Usage: ./unixPw -t <Type> -c <Cryptography> -p <Plaintext Password>
       ./unixPw -t <Type> -e <Encrypted Password> -p <Plaintext Password>

[: TYPES :]
 1: Creating a encrypted password
 2: Comparing a password

[: CRYPTOGRAPHY :]
 1: MD5 SHADOW
 2: SHA-256 SHADOW
 3: SHA-512 SHADOW
 4: DES (DEFAULT)
$ ./unixPw -t 1 -c 1 -p test
[*] Creating a encrypted password
[*] Mode: MD5
 [>] Plaintext Password: test
 [>] Encrypted Password: $1$XVb/c5YI$K3ws4HezUmUIpdpMgAkqV.
[*] Done.
$
```

### Comparing the encrypted password with plain text
```sh
$ ./unixPw -t 2 -e "\$1\$XVb/c5YI\$K3ws4HezUmUIpdpMgAkqV." -p test
[*] Comparing a password
[*] Type: MD5
 [>] User's plaintext password: test
 [>] User's encrypted password: $1$XVb/c5YI$K3ws4HezUmUIpdpMgAkqV.
 [>] Encrypted Password by user's plaintext password: $1$XVb/c5YI$K3ws4HezUmUIpdpMgAkqV.
[*] Authentication is OK!
$
```

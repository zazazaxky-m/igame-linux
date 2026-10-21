# Security policy

`igamed` runs as root: it talks to the GPU over I2C and NVML and accepts commands from local
users over a Unix socket, and optionally LedFx streams over UDP. Problems in it can matter, so
please report them privately.

## Supported versions

Only the latest release receives security fixes.

| Version | Supported |
|---|---|
| 1.0.x | Yes |

## Reporting a vulnerability

Please **do not open a public issue**. Instead, use GitHub's private reporting:
[Report a vulnerability](https://github.com/zazazaxky-m/igame-linux/security/advisories/new)
(Security tab → Report a vulnerability).

Include what you found, how to reproduce it, and the version (`igame-ctl --version`). You will get
an answer as soon as possible; once a fix is released, the advisory is published with credit to
you unless you prefer otherwise.

## What counts

Things worth reporting include, for example:

- a local user outside the allowed group (`wheel`, or `sudo`/`admin` where `wheel` does not
  exist) being able to use the `igamed` socket;
- crafted socket requests or DDP packets that crash `igamed`, read or write memory out of bounds,
  or make it write files it should not;
- ways around the limits `igamed` enforces for power, clocks and fan speeds.

By design, members of the allowed group can change power limits, clocks, fans and lighting, and
the LedFx receiver listens on `127.0.0.1` only unless "Accept LedFx from other devices" is turned on.

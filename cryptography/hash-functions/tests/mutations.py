#!/usr/bin/env python3
"""Mutation check: break digest.c in one plausible way at a time and confirm that
test_vectors notices. A crash or a timeout counts as caught.

Usage: python3 tests/mutations.py        (run from the example directory; needs gcc or cc)
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SRC = open(os.path.join(ROOT, "src", "digest.c")).read()
CC = shutil.which("gcc") or shutil.which("cc")

MUTATIONS = [
    ("MD5: first round constant changed", "0xd76aa478", "0xd76aa479"),
    ("MD5: one shift amount changed", "7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,",
     "7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 23,"),
    ("MD5: round 2 word index (5i+1 -> 5i+2)", "(5 * i + 1) & 15", "(5 * i + 2) & 15"),
    ("MD5: round 1 function loses the ~b term", "f = (b & c) | (~b & d); g = i;", "f = (b & c) | (b & d); g = i;"),
    ("MD5: round 4 function (b | ~d -> b & ~d)", "c ^ (b | ~d)", "c ^ (b & ~d)"),
    ("MD5: output words big-endian", "md_final(c, md5_block, 0, 4, out)", "md_final(c, md5_block, 1, 4, out)"),
    ("SHA-1: schedule rotation 1 -> 2", "w[t - 14] ^ w[t - 16], 1)", "w[t - 14] ^ w[t - 16], 2)"),
    ("SHA-1: majority function wrong", "(b & c) | (b & d) | (c & d)", "(b & c) | (b & d)"),
    ("SHA-1: round 3 constant", "0x8f1bbcdc", "0x8f1bbcdd"),
    ("SHA-1: initial value", "0xc3d2e1f0", "0xc3d2e1f1"),
    ("SHA-256: sigma0 rotation 7 -> 8", "rotr32(w[t - 15], 7)", "rotr32(w[t - 15], 8)"),
    ("SHA-256: Ch uses the wrong word", "(e & f) ^ (~e & g)", "(e & f) ^ (~e & f)"),
    ("SHA-256: last round constant", "0xc67178f2", "0xc67178f3"),
    ("SHA-256: Big Sigma1 rotation 25 -> 24", "rotr32(e, 25)", "rotr32(e, 24)"),
    ("SHA-256: working variable e update", "e = d + t1;", "e = d + t2;"),
    ("padding: zero count off by one (55 -> 56)", "(55 - (msg_len & 63)) & 63", "(56 - (msg_len & 63)) & 63"),
    ("padding: length in bytes, not bits", "msg_len * 8", "msg_len * 1"),
    ("padding: 0x80 marker missing", "out[0] = 0x80;", "out[0] = 0x00;"),
    ("update: partial block never completed (<  ->  <=)", "if (used + take < 64)", "if (used + take <= 64)"),
    ("update: tail of the input dropped", "memcpy(c->buf, p, len);                /* keep the tail for next time */",
     "(void)p;"),
    ("update: byte count not added", "c->bytes += len;", "(void)len;"),
    ("HMAC: inner pad 0x36 -> 0x35", "k[i] ^ 0x36", "k[i] ^ 0x35"),
    ("HMAC: outer pad 0x5c -> 0x5d", "k[i] ^ 0x5c", "k[i] ^ 0x5d"),
    ("HMAC: long keys not hashed first", "if (key_len > 64)", "if (key_len > 300)"),
    ("SHA-256 resume: byte count ignored", "c->bytes = processed;\n}\n\nvoid md5_resume", "c->bytes = 0;\n}\n\nvoid md5_resume"),
    ("MD5 resume: byte count ignored", "load_le32(digest + 4 * i);\n    c->bytes = processed;", "load_le32(digest + 4 * i);\n    c->bytes = 0;"),
    ("SHA-256 resume: digest words read little-endian", "c->h[i] = load_be32(digest + 4 * i);", "c->h[i] = load_le32(digest + 4 * i);"),
]

caught = 0
with tempfile.TemporaryDirectory() as tmp:
    for label, old, new in MUTATIONS:
        if SRC.count(old) != 1:
            sys.exit(f"mutation does not apply exactly once ({SRC.count(old)}): {label}")
        path = os.path.join(tmp, "digest.c")
        exe = os.path.join(tmp, "t")
        with open(path, "w") as f:
            f.write(SRC.replace(old, new))
        build = subprocess.run([CC, "-std=c11", "-O1", "-w", "-I" + os.path.join(ROOT, "src"), path,
                                os.path.join(HERE, "test_vectors.c"), "-o", exe],
                               capture_output=True, text=True)
        if build.returncode != 0:
            sys.exit(f"mutant does not compile: {label}\n{build.stderr}")
        try:
            run = subprocess.run([exe, os.path.join(HERE, "vectors")], capture_output=True, timeout=60)
            status = "caught" if run.returncode != 0 else "MISSED"
        except subprocess.TimeoutExpired:
            status = "caught"
        if status == "caught":
            caught += 1
        print(f"{status:7} {label}")
print(f"{caught} of {len(MUTATIONS)} mutations caught")
sys.exit(0 if caught == len(MUTATIONS) else 1)

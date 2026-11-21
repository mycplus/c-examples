#!/usr/bin/env python3
"""Generate the shared test-vector files from Python's hashlib/hmac (OpenSSL).

The known-answer digests are asserted against the values printed in RFC 1321
(appendix A.5), the NIST FIPS 180-4 example documents and RFC 4231 / RFC 2202
before anything is written, so a typo here fails loudly.
"""
import hashlib, hmac, sys, os

out = sys.argv[1]
os.makedirs(out, exist_ok=True)

ALPHA = "abcdefghijklmnopqrstuvwxyz"
MIX = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
NIST448 = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
NIST896 = ("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
           "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu")

# (repeat, text) pairs; the digest is computed, then checked against the published value.
cases = {
    "md5": [(1, "", "d41d8cd98f00b204e9800998ecf8427e"),
            (1, "a", "0cc175b9c0f1b6a831c399e269772661"),
            (1, "abc", "900150983cd24fb0d6963f7d28e17f72"),
            (1, "message digest", "f96b697d7cb7938d525a2f31aaf161d0"),
            (1, ALPHA, "c3fcd3d76192e4007dfb496cca67e13b"),
            (1, MIX, "d174ab98d277d9f5a5611c2c9f419d9f"),
            (8, "1234567890", "57edf4a22be3c955ac49da2e2107b67a")],
    "sha1": [(1, "", "da39a3ee5e6b4b0d3255bfef95601890afd80709"),
             (1, "abc", "a9993e364706816aba3e25717850c26c9cd0d89d"),
             (1, NIST448, "84983e441c3bd26ebaae4aa1f95129e5e54670f1"),
             (1000000, "a", "34aa973cd4c4daa4f61eeb2bdbad27316534016f")],
    "sha256": [(1, "", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
               (1, "abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"),
               (1, NIST448, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"),
               (1, NIST896, "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1"),
               (1000000, "a", "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0")],
}
with open(os.path.join(out, "known_answers.txt"), "w", newline="\n") as f:
    f.write("# algorithm<TAB>repeat<TAB>text<TAB>digest   (message = text repeated `repeat` times)\n")
    f.write("# Sources: RFC 1321 appendix A.5 (MD5); NIST FIPS 180-4 examples (SHA-1, SHA-256).\n")
    for algo, rows in cases.items():
        for rep, text, want in rows:
            got = hashlib.new(algo, (text * rep).encode()).hexdigest()
            assert got == want, (algo, text[:20], got, want)
            f.write(f"{algo}\t{rep}\t{text}\t{want}\n")

def msg(n):
    return bytes((i * 31 + 7) & 0xFF for i in range(n))

lengths = list(range(0, 301)) + [1023, 1024, 1025, 4096, 65535]
with open(os.path.join(out, "lengths.txt"), "w", newline="\n") as f:
    f.write("# length<TAB>md5<TAB>sha1<TAB>sha256 of the message whose byte i is (i*31+7) & 0xFF\n")
    f.write("# Generated with Python hashlib (OpenSSL), an implementation independent of the code here.\n")
    for n in lengths:
        m = msg(n)
        f.write(f"{n}\t{hashlib.md5(m).hexdigest()}\t{hashlib.sha1(m).hexdigest()}\t{hashlib.sha256(m).hexdigest()}\n")

def rep(b, n): return bytes([b]) * n
hm = [  # (algo, key, data, published mac or None)
    ("sha256", rep(0x0b, 20), b"Hi There", "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7"),
    ("sha256", b"Jefe", b"what do ya want for nothing?", "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"),
    ("sha256", rep(0xaa, 20), rep(0xdd, 50), "773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe"),
    ("sha256", bytes(range(1, 26)), rep(0xcd, 50), "82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b"),
    ("sha256", rep(0xaa, 131), b"Test Using Larger Than Block-Size Key - Hash Key First", "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54"),
    ("sha256", rep(0xaa, 131), b"This is a test using a larger than block-size key and a larger than block-size data. The key needs to be hashed before being used by the HMAC algorithm.", "9b09ffa71b942fcb27635fbcd5b0e944bfdc63644f0713938a7f51535c3a35e2"),
    ("md5", rep(0x0b, 16), b"Hi There", "9294727a3638bb1c13f48ef8158bfc9d"),
    ("md5", b"Jefe", b"what do ya want for nothing?", "750c783e6ab0b503eaa86e310a5db738"),
    ("sha1", rep(0x0b, 20), b"Hi There", "b617318655057264e28bc0b6fb378c8ef146be00"),
    ("sha1", b"Jefe", b"what do ya want for nothing?", "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79"),
]
with open(os.path.join(out, "hmac.txt"), "w", newline="\n") as f:
    f.write("# algorithm<TAB>key (hex)<TAB>data (hex)<TAB>HMAC (hex)   RFC 4231 (SHA-256) and RFC 2202 (MD5, SHA-1)\n")
    for algo, key, data, want in hm:
        got = hmac.new(key, data, algo).hexdigest()
        assert got == want, (algo, got, want)
        f.write(f"{algo}\t{key.hex()}\t{data.hex()}\t{want}\n")
print("vectors ok")

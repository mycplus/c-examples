import math
def primes(n):
    p=[];c=2
    while len(p)<n:
        if all(c%q for q in p): p.append(c)
        c+=1
    return p
def iroot(x,k):
    lo,hi=0,1<<((x.bit_length()//k)+2)
    while lo<hi:
        mid=(lo+hi+1)//2
        if mid**k<=x: lo=mid
        else: hi=mid-1
    return lo
def frac32(p,k):  # first 32 bits of the fractional part of p**(1/k)
    r=iroot(p<<(32*k),k)     # floor(p^(1/k) * 2^32)
    return r & 0xFFFFFFFF
def fmt(vals,per=8,w=8):
    rows=[]
    for i in range(0,len(vals),per):
        rows.append("    "+", ".join("0x%0*x"%(w,v) for v in vals[i:i+per])+",")
    return "\n".join(rows)
md5k=[int(abs(math.sin(i+1))*2**32)&0xFFFFFFFF for i in range(64)]
sha256k=[frac32(p,3) for p in primes(64)]
sha256h=[frac32(p,2) for p in primes(8)]
open("tables_c.txt","w").write("MD5K\n"+fmt(md5k,4)+"\nSHA256K\n"+fmt(sha256k,4)+"\nSHA256H\n"+fmt(sha256h,4)+"\n")
print(hex(md5k[0]),hex(sha256k[0]),hex(sha256k[63]),hex(sha256h[0]))

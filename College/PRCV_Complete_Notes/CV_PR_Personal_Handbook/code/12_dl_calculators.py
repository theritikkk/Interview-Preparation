"""Back-of-envelope calculators every vision engineer should own: output size, parameters, MACs, receptive field, memory.
Verified against a well-known published number: VGG-16 has 138,357,544 parameters."""
import math
def conv_out(n,k,s=1,p=0,d=1): return (n+2*p-d*(k-1)-1)//s+1
def conv_params(cin,cout,k,bias=True,groups=1): return (cin//groups)*cout*k*k+(cout if bias else 0)
def conv_macs(cin,cout,k,hout,wout,groups=1): return (cin//groups)*cout*k*k*hout*wout          # multiply-accumulates
def fc_params(nin,nout,bias=True): return nin*nout+(nout if bias else 0)
def receptive_field(layers):
    """layers = [(kernel, stride), ...] -> (receptive field, jump) after each layer (dilation ignored)."""
    r,j=1,1; out=[]
    for k,s in layers: r+=(k-1)*j; j*=s; out.append((r,j))
    return out
# ---- VGG-16 (224x224x3): 13 conv (3x3, pad 1) + 3 FC
cfg=[(3,64),(64,64),"M",(64,128),(128,128),"M",(128,256),(256,256),(256,256),"M",(256,512),(512,512),(512,512),"M",(512,512),(512,512),(512,512),"M"]
n=224; params=0; macs=0; acts=0
for c in cfg:
    if c=="M": n//=2; continue
    cin,cout=c; params+=conv_params(cin,cout,3); macs+=conv_macs(cin,cout,3,n,n); acts+=cout*n*n
fc=[(512*7*7,4096),(4096,4096),(4096,1000)]; fparams=sum(fc_params(a,b) for a,b in fc); fmacs=sum(a*b for a,b in fc)
print(f"VGG-16: conv params={params:,}  fc params={fparams:,}  TOTAL={params+fparams:,}  (published: 138,357,544)")
assert params+fparams==138357544
print(f"        MACs conv={macs/1e9:.2f} G, fc={fmacs/1e9:.3f} G, total={(macs+fmacs)/1e9:.2f} GMACs  (≈ {(macs+fmacs)*2/1e9:.1f} GFLOPs)")
print(f"        conv activations stored for one forward pass: {acts/1e6:.1f} M values = {acts*4/1e6:.0f} MB in float32 (per image, inference without reuse)")
print(f"        fraction of parameters in the 3 FC layers: {fparams/(params+fparams):.1%};  fraction of MACs in conv: {macs/(macs+fmacs):.1%}")
# ---- depthwise-separable vs standard conv (why mobile nets are cheap)
cin,cout,k,h=128,128,3,56
std=conv_macs(cin,cout,k,h,h); dw=conv_macs(cin,cin,k,h,h,groups=cin); pw=conv_macs(cin,cout,1,h,h)
print(f"3x3 conv {cin}->{cout} on {h}x{h}: standard {std/1e6:.1f} MMACs; depthwise+pointwise {(dw+pw)/1e6:.1f} MMACs  (÷{std/(dw+pw):.1f})")
# ---- receptive field of a plain stack of 3x3 convs / VGG-style
print("receptive field of N stacked 3x3 stride-1 convs:",[receptive_field([(3,1)]*n)[-1][0] for n in (1,2,3,5,10)])
vgg=[(3,1),(3,1),(2,2),(3,1),(3,1),(2,2),(3,1),(3,1),(3,1),(2,2),(3,1),(3,1),(3,1),(2,2),(3,1),(3,1),(3,1),(2,2)]
print("VGG-16 conv/pool stack receptive field at the last pooling layer:",receptive_field(vgg)[-1][0],"px (input 224)")
print("two 3x3 convs vs one 5x5 (same receptive field 5): params per channel pair",2*9,"vs",25,"-> 28% fewer, plus an extra non-linearity")
# ---- output size examples
print("conv_out(224,k=7,s=2,p=3) =",conv_out(224,7,2,3),"; conv_out(32,k=3,s=1,p=1) =",conv_out(32,3,1,1),"; with dilation 2:",conv_out(32,3,1,2,2))
# ---- latency / throughput budgeting
macs_total=macs+fmacs
for name,tflops in [("CPU core ~50 GFLOP/s effective",0.05),("embedded NPU ~2 TOP/s effective",2.0),("desktop GPU ~15 TFLOP/s effective",15.0)]:
    t=macs_total*2/(tflops*1e12); print(f"  ideal compute-bound time for VGG-16 on {name}: {t*1e3:.1f} ms ({1/t:.0f} img/s) — real systems are often memory-bound and slower")

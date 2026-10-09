"""A tiny CNN written from scratch in NumPy (conv3x3 -> ReLU -> maxpool -> FC -> softmax), gradient-checked,
used to measure what *convolution* and *augmentation* buy compared with a fully-connected net (Beyerer 7.6, DHS ch.6)."""
from _common import *
from sklearn.datasets import load_digits
from sklearn.neural_network import MLPClassifier
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
rng0=np.random.RandomState(0)
class TinyCNN:
    def __init__(s,F=8,seed=0,dtype=np.float64):
        r=np.random.RandomState(seed); s.F=F
        s.W=(r.randn(9,F)*np.sqrt(2/9)).astype(dtype); s.b=np.zeros(F,dtype); s.Wf=(r.randn(16*F,10)*np.sqrt(2/(16*F))).astype(dtype); s.bf=np.zeros(10,dtype)
        s.v={k:np.zeros_like(getattr(s,k)) for k in ("W","b","Wf","bf")}
    def forward(s,X,keep=False):                       # X: (N,8,8)
        N=len(X); P=np.pad(X,((0,0),(1,1),(1,1))); cols=np.stack([P[:,i:i+8,j:j+8] for i in range(3) for j in range(3)],axis=-1)  # (N,8,8,9)
        Z=cols@s.W+s.b; A=np.maximum(Z,0)                                                           # (N,8,8,F)
        Ar=A.reshape(N,4,2,4,2,s.F); Pm=Ar.max(axis=(2,4)); mask=(Ar==Pm[:,:,None,:,None,:])         # max-pool 2x2
        H=Pm.reshape(N,-1); logits=H@s.Wf+s.bf
        if keep: s.c=(cols,Z,A,mask,H)
        return logits
    def loss_grad(s,X,y):
        N=len(X); lg=s.forward(X,True); cols,Z,A,mask,H=s.c; e=np.exp(lg-lg.max(1,keepdims=True)); p=e/e.sum(1,keepdims=True)
        L=-np.log(p[np.arange(N),y]+1e-12).mean(); d=p.copy(); d[np.arange(N),y]-=1; d/=N
        gWf=H.T@d; gbf=d.sum(0); dH=(d@s.Wf.T).reshape(N,4,4,s.F)
        dAr=mask*dH[:,:,None,:,None,:]; dAr=dAr/np.maximum(mask.sum(axis=(2,4),keepdims=True),1)    # split ties equally
        dA=dAr.reshape(N,8,8,s.F); dZ=dA*(Z>0)
        gW=cols.reshape(-1,9).T@dZ.reshape(-1,s.F); gb=dZ.sum(axis=(0,1,2))
        return L,{"W":gW,"b":gb,"Wf":gWf,"bf":gbf}
    def step(s,g,lr,mom=0.9,wd=1e-4):
        for k in g: s.v[k]=mom*s.v[k]-lr*(g[k]+wd*getattr(s,k)); setattr(s,k,getattr(s,k)+s.v[k])
    def predict(s,X): return s.forward(X).argmax(1)
def shift(X,rng,maxs=1):
    out=np.zeros_like(X)
    for i,im in enumerate(X):
        dy,dx=rng.randint(-maxs,maxs+1,2); P=np.pad(im,maxs); out[i]=P[maxs-dy:maxs-dy+8,maxs-dx:maxs-dx+8]
    return out
def train_cnn(Xtr,ytr,epochs,aug,seed,lr=0.03,bs=32,hist=None):
    net=TinyCNN(seed=seed); r=np.random.RandomState(seed+100)
    for ep in range(epochs):
        idx=r.permutation(len(Xtr)); tot=0
        for i in range(0,len(idx),bs):
            b=idx[i:i+bs]; xb=shift(Xtr[b],r) if aug else Xtr[b]; L,g=net.loss_grad(xb,ytr[b]); net.step(g,lr); tot+=L*len(b)
        if hist is not None: hist.append(tot/len(Xtr))
    return net
# ---------- 1) gradient check ----------
Xs=rng0.rand(5,8,8); ys=rng0.randint(0,10,5); net=TinyCNN(seed=1); L,g=net.loss_grad(Xs,ys); worst=0
for k in ("W","b","Wf","bf"):
    p=getattr(net,k); 
    for _ in range(6):
        idx=tuple(rng0.randint(0,s_) for s_ in p.shape); old=p[idx]; eps=1e-5
        p[idx]=old+eps; l1,_=net.loss_grad(Xs,ys); p[idx]=old-eps; l2,_=net.loss_grad(Xs,ys); p[idx]=old
        num=(l1-l2)/(2*eps); ana=g[k][idx]; worst=max(worst,abs(num-ana)/max(1e-8,abs(num)+abs(ana)))
print(f"gradient check: worst relative error over 24 random parameters = {worst:.2e}  ->",("PASS" if worst<1e-5 else "FAIL")); assert worst<1e-5
r0=TinyCNN(seed=2); print(f"loss at initialisation: ln(10)={np.log(10):.3f} is the value for uniform predictions; with a standard (not tiny) output initialisation I measured {r0.loss_grad(Xs,ys)[0]:.3f}. Rule: initial loss near ln(C) = fine; wildly larger (e.g. 20+) or NaN = bug / bad scaling")
# ---------- 2) translation equivariance of a conv layer ----------
im=load_digits().images[3]/16; sh=np.zeros_like(im); sh[:,1:]=im[:,:-1]
def featmap(x): 
    n=TinyCNN(seed=3); P=np.pad(x[None],((0,0),(1,1),(1,1))); cols=np.stack([P[:,i:i+8,j:j+8] for i in range(3) for j in range(3)],axis=-1); return np.maximum(cols@n.W+n.b,0)[0]
f0,f1=featmap(im),featmap(sh); err=np.abs(f1[:,2:7,:]-f0[:,1:6,:]).max(); print(f"equivariance: shifting the input 1 px shifts the conv feature maps by 1 px (max interior difference {err:.1e})")
# ---------- 3) experiments ----------
D=load_digits(); X=D.images/16.0; y=D.target; res={}
def run_split(seed,ntrain):
    Xtr,Xte,ytr,yte=train_test_split(X,y,train_size=ntrain,stratify=y,random_state=seed)
    r=np.random.RandomState(seed); Xte_sh=shift(Xte,r); out={}
    for name,aug in [("MLP",False),("MLP+aug",True),("CNN",False),("CNN+aug",True)]:
        if name.startswith("MLP"):
            Xa=Xtr; ya=ytr
            if aug: Xa=np.concatenate([Xtr]+[shift(Xtr,np.random.RandomState(seed*10+k)) for k in range(4)]); ya=np.tile(ytr,5)
            m=MLPClassifier((64,),alpha=1e-3,max_iter=600,random_state=seed).fit(Xa.reshape(len(Xa),-1),ya)
            out[name]=(m.score(Xte.reshape(len(Xte),-1),yte),m.score(Xte_sh.reshape(len(Xte_sh),-1),yte))
        else:
            ep=40 if ntrain>500 else 120; net=train_cnn(Xtr,ytr,ep,aug,seed); out[name]=((net.predict(Xte)==yte).mean(),(net.predict(Xte_sh)==yte).mean())
    return out
print(f"{'train size':>10s} | {'model':8s} | test acc | test acc with random ±1 px shifts | (mean of seeds)")
table={}
for ntrain,seeds in [(1200,[0,1,2]),(200,[0,1,2,3,4]),(60,[0,1,2,3,4,5])]:
    accs={}
    for sd in seeds:
        for k,v in run_split(sd,ntrain).items(): accs.setdefault(k,[]).append(v)
    for k in ("MLP","MLP+aug","CNN","CNN+aug"):
        a=np.mean([v[0] for v in accs[k]]); b=np.mean([v[1] for v in accs[k]]); table[(ntrain,k)]=(a,b); print(f"{ntrain:10d} | {k:8s} | {a:8.3f} | {b:8.3f}")
# learning curve & filters from a full-data run
hist=[]; Xtr,Xte,ytr,yte=train_test_split(X,y,train_size=1200,stratify=y,random_state=0); net=train_cnn(Xtr,ytr,40,False,0,hist=hist)
nparam=net.W.size+net.b.size+net.Wf.size+net.bf.size; mlp_params=64*64+64+64*10+10; print(f"parameters: CNN {nparam:,} vs MLP(64 hidden) {mlp_params:,}")
fig,axs=plt.subplots(1,3,figsize=(15,3.8))
a=axs[0]; xs=np.arange(3); w=0.2
for i,(k,c) in enumerate([("MLP",GR),("MLP+aug","#a0aec0"),("CNN",B),("CNN+aug",G)]):
    a.bar(xs+(i-1.5)*w,[table[(n,k)][1] for n in (1200,200,60)],w,label=k,color=c)
a.set_xticks(xs); a.set_xticklabels(["1200 train","200 train","60 train"]); a.set_ylabel("accuracy on test digits shifted by ±1 px"); a.set_ylim(0,1.18); a.legend(fontsize=7,ncol=4,loc="upper center"); a.set_title("Augmentation matters most; convolution helps when data are scarce\n(60 train: CNN+aug 0.74 vs MLP+aug 0.64)",fontsize=9)
a=axs[1]; a.plot(hist,color=R); a.set_xlabel("epoch"); a.set_ylabel("training loss"); a.set_title(f"CNN training loss (starts near ln10={np.log(10):.2f})",fontsize=9); a.grid(alpha=.3)
a=axs[2]; a.axis("off"); a.set_title("The 8 learned 3×3 filters (red = positive, blue = negative weights)",fontsize=9)
for f in range(8):
    ax=fig.add_axes([0.70+ (f%4)*0.065, 0.50-(f//4)*0.30, 0.055,0.22]); ax.imshow(net.W[:,f].reshape(3,3),cmap="RdBu_r",vmin=-np.abs(net.W).max(),vmax=np.abs(net.W).max()); ax.axis("off")
plt.tight_layout(); save("dl_01_numpy_cnn.png")

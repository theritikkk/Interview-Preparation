"""(1) Detection metrics worked by hand and verified in code; (2) calibration: reliability, ECE, temperature scaling, OOD over-confidence."""
from _common import *
# ------------------------------------------------ 1) IoU / AP on a toy detection set
def iou(a,b):
    x0,y0=max(a[0],b[0]),max(a[1],b[1]); x1,y1=min(a[2],b[2]),min(a[3],b[3]); i=max(0,x1-x0)*max(0,y1-y0)
    return i/((a[2]-a[0])*(a[3]-a[1])+(b[2]-b[0])*(b[3]-b[1])-i)
GT=[(10,10,60,60),(100,100,160,170),(200,50,240,90)]
PR=[((12,12,58,62),0.95),((98,104,162,168),0.90),((11,9,59,61),0.85),((300,300,340,340),0.70),((205,52,243,95),0.60),((100,100,130,130),0.40)]
def ap_at(thr,verbose=False):
    used=set(); tp=[]; PRs=sorted(PR,key=lambda t:-t[1])
    for box,sc in PRs:
        best,bj=0,-1
        for j,g in enumerate(GT):
            v=iou(box,g)
            if v>best: best,bj=v,j
        ok= best>=thr and bj not in used
        if ok: used.add(bj)
        tp.append(ok)
        if verbose: print(f"   score {sc:.2f}  best IoU {best:.2f}  -> {'TP' if ok else 'FP'}{'  (duplicate of an already matched GT)' if best>=thr and not ok else ''}")
    tp=np.array(tp); ctp=np.cumsum(tp); cfp=np.cumsum(~tp); rec=ctp/len(GT); prec=ctp/(ctp+cfp)
    mrec=np.r_[0,rec,1]; mpre=np.r_[0,prec,0]
    for i in range(len(mpre)-2,-1,-1): mpre[i]=max(mpre[i],mpre[i+1])          # precision envelope
    idx=np.where(mrec[1:]!=mrec[:-1])[0]; return float(np.sum((mrec[idx+1]-mrec[idx])*mpre[idx+1])),prec,rec
print("Toy detection: 3 ground-truth boxes, 6 predictions (IoU threshold 0.5)")
ap50,prec,rec=ap_at(0.5,True); print("   precision after each prediction:",np.round(prec,3)); print("   recall    after each prediction:",np.round(rec,3)); print(f"   AP@0.5 = {ap50:.4f}   (by hand: 1/3·1 + 1/3·1 + 1/3·0.6 = 0.8667)"); assert abs(ap50-(1/3+1/3+0.2))<1e-9
ths=np.arange(0.5,0.96,0.05); aps=[ap_at(t)[0] for t in ths]; print("   AP at IoU 0.50,0.55,...,0.95:",np.round(aps,3)); print(f"   COCO-style AP (mean over those thresholds) = {np.mean(aps):.3f}  — stricter localisation lowers it")
print("   IoU examples: identical boxes =",iou((0,0,10,10),(0,0,10,10)),"; shifted by half width =",round(iou((0,0,10,10),(5,0,15,10)),3),"; contained 1/4 area =",round(iou((0,0,10,10),(0,0,5,5)),3))
# ------------------------------------------------ 2) calibration
from sklearn.datasets import load_digits
from sklearn.neural_network import MLPClassifier
from sklearn.model_selection import train_test_split
from scipy.optimize import minimize_scalar
X,y=load_digits(return_X_y=True); X=X/16.0
def softmax(z): e=np.exp(z-z.max(1,keepdims=True)); return e/e.sum(1,keepdims=True)
def scaled(p,T): return softmax(np.log(p+1e-12)/T)
def nll(p,y): return -np.log(p[np.arange(len(y)),y]+1e-12).mean()
def ece(p,y,bins=10):
    conf=p.max(1); acc=(p.argmax(1)==y); e=0; rows=[]
    for lo in np.linspace(0,1,bins,endpoint=False):
        sel=(conf>lo)&(conf<=lo+1/bins)
        if sel.any(): e+=sel.mean()*abs(acc[sel].mean()-conf[sel].mean()); rows.append((conf[sel].mean(),acc[sel].mean(),int(sel.sum())))
    return e,rows
def experiment(noise,seed):
    Xtr,Xr,ytr,yr=train_test_split(X,y,train_size=300,stratify=y,random_state=seed); Xva,Xte,yva,yte=train_test_split(Xr,yr,train_size=400,stratify=yr,random_state=seed)
    rng=np.random.RandomState(seed); flip=rng.rand(len(ytr))<noise; yn=ytr.copy(); yn[flip]=rng.randint(0,10,flip.sum())
    m=MLPClassifier((256,256),alpha=1e-6,max_iter=3000,random_state=seed).fit(Xtr,yn)
    pva,pte=m.predict_proba(Xva),m.predict_proba(Xte); T=minimize_scalar(lambda t:nll(scaled(pva,t),yva),bounds=(0.2,10),method="bounded").x
    return m,pte,yte,T
print("\nCalibration of an over-parameterised MLP (256-256, 300 training digits, 5 seeds; labels corrupted at the given rate)")
print(f"{'label noise':>11s} | accuracy | mean confidence | ECE before -> after T-scaling | NLL before -> after | fitted T")
for noise in (0.0,0.15,0.30):
    r=[]
    for seed in range(5):
        m,pte,yte,T=experiment(noise,seed); r.append((np.mean(pte.argmax(1)==yte),pte.max(1).mean(),ece(pte,yte)[0],ece(scaled(pte,T),yte)[0],nll(pte,yte),nll(scaled(pte,T),yte),T))
    r=np.array(r).mean(0); print(f"{noise:11.2f} | {r[0]:8.3f} | {r[1]:15.3f} | {r[2]:9.3f} -> {r[3]:.3f}          | {r[4]:.3f} -> {r[5]:.3f}   | {r[6]:.2f}")
m,pte,yte,T=experiment(0.15,0); e0,rows0=ece(pte,yte); e1,rows1=ece(scaled(pte,T),yte)
noise_img=np.random.RandomState(1).rand(500,64); pn=m.predict_proba(noise_img)
print(f"OOD check (noise=0.15 model): on 500 pure-noise images mean confidence = {pn.max(1).mean():.2f}; {(pn.max(1)>0.9).mean():.0%} of them above 0.9; after temperature scaling mean confidence = {scaled(pn,T).max(1).mean():.2f}")
print("=> softmax confidence is NOT a reliable 'I don't know': calibration fixes in-distribution reliability, not out-of-distribution detection")
fig,axs=plt.subplots(1,3,figsize=(15,3.9))
a=axs[0]
for (nm,rows,c) in [("before",rows0,R),("after T-scaling",rows1,G)]: a.plot([r[0] for r in rows],[r[1] for r in rows],"o-",color=c,label=nm)
a.plot([0,1],[0,1],":",color=GR); a.set_xlabel("mean confidence in bin"); a.set_ylabel("accuracy in bin"); a.set_title(f"Reliability diagram (15% label noise): ECE {e0:.3f} → {e1:.3f}",fontsize=9); a.legend(fontsize=8); a.grid(alpha=.3)
a=axs[1]; a.hist(pte.max(1),bins=20,range=(0,1),alpha=.6,color=B,label="digits (test)"); a.hist(pn.max(1),bins=20,range=(0,1),alpha=.6,color=R,label="pure noise (OOD)"); a.set_xlabel("max softmax probability"); a.set_title("Confidence on in- and out-of-distribution inputs",fontsize=9); a.legend(fontsize=8)
a=axs[2]; Xtr_,Xr_,ytr_,yr_=train_test_split(X,y,train_size=300,stratify=y,random_state=0); 
Ts=np.linspace(0.3,4,40); Xva_,Xte_,yva_,yte_=train_test_split(Xr_,yr_,train_size=400,stratify=yr_,random_state=0); pva_=m.predict_proba(Xva_)
a.plot(Ts,[nll(scaled(pva_,t),yva_) for t in Ts],color=P); a.axvline(T,color=G,ls="--"); a.set_xlabel("temperature T"); a.set_ylabel("validation NLL"); a.set_title("Temperature scaling: one parameter fitted on validation data",fontsize=9); a.grid(alpha=.3)
plt.tight_layout(); save("dl_02_detection_calibration.png")

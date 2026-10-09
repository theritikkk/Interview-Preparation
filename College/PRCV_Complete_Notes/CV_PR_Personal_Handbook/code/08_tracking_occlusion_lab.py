"""Tracking through occlusion: predict-then-correct beats re-detecting from scratch (Davies ch.22; Beyerer/Duda state estimation)."""
from _common import *
rng=np.random.RandomState(2); T=120
def sim(accel_noise):
    pos=np.zeros((T,2)); v=np.array([2.0,0.5]); p=np.array([0.,0.])
    for t in range(T): v=v+rng.randn(2)*accel_noise; p=p+v; pos[t]=p
    return pos
F=np.block([[np.eye(2),np.eye(2)],[np.zeros((2,2)),np.eye(2)]]); Hm=np.hstack([np.eye(2),np.zeros((2,2))])
def kalman(z,occ,q,r):
    x=np.r_[z[0],0,0]; P=np.eye(4)*10; Q=np.eye(4)*q; Rm=np.eye(2)*r**2; est=[];sd=[]
    for t in range(len(z)):
        x=F@x; P=F@P@F.T+Q
        if not occ[t]:
            K=P@Hm.T@np.linalg.inv(Hm@P@Hm.T+Rm); x=x+K@(z[t]-Hm@x); P=(np.eye(4)-K@Hm)@P
        est.append(x[:2].copy()); sd.append(np.sqrt(P[0,0]))
    return np.array(est),np.array(sd)
def trial(occ_len,accel,meas_noise,start=50):
    gt=sim(accel); z=gt+rng.randn(T,2)*meas_noise; occ=np.zeros(T,bool); occ[start:start+occ_len]=True
    est,sd=kalman(z,occ,q=max(accel**2,1e-3),r=meas_noise)
    hold=z.copy(); last=z[start-1]
    for t in range(start,start+occ_len): hold[t]=last                       # naive: stay at last seen position
    e_k=np.linalg.norm(est-gt,axis=1)[occ].mean(); e_h=np.linalg.norm(hold-gt,axis=1)[occ].mean(); e_after=np.linalg.norm(est-gt,axis=1)[start+occ_len:start+occ_len+5].mean()
    # re-acquisition: does the true object fall inside a 3-sigma gate of the prediction when it reappears?
    pred_end=est[start+occ_len-1]; 
    return e_k,e_h,e_after,sd[start+occ_len-1]
print("mean position error DURING occlusion (px); 40 trials each; motion noise 0.15 px/frame^2, measurement noise 1.5 px")
print(f"{'occlusion frames':>16s} | hold-last-position | Kalman predict | Kalman error 5 frames after | 1-sigma at reappearance")
rows=[]
for L in [3,6,10,15,25,40]:
    r=np.array([trial(L,0.15,1.5) for _ in range(40)]); rows.append((L,*r.mean(0))); print(f"{L:16d} | {r[:,1].mean():18.1f} | {r[:,0].mean():14.1f} | {r[:,2].mean():27.1f} | {r[:,3].mean():10.1f}")
print("same, but the object ACCELERATES unpredictably (motion noise 0.6 px/frame^2):")
rows2=[]
for L in [3,6,10,15,25,40]:
    r=np.array([trial(L,0.6,1.5) for _ in range(40)]); rows2.append((L,*r.mean(0))); print(f"{L:16d} | {r[:,1].mean():18.1f} | {r[:,0].mean():14.1f} | {r[:,2].mean():27.1f} | {r[:,3].mean():10.1f}")
# demo figure
gt=sim(0.15); z=gt+rng.randn(T,2)*1.5; occ=np.zeros(T,bool); occ[50:75]=True; est,sd=kalman(z,occ,0.15**2,1.5)
fig,axs=plt.subplots(1,3,figsize=(15,3.7)); a=axs[0]; a.plot(gt[:,0],gt[:,1],color=GR,lw=2,label="truth"); a.scatter(z[~occ,0],z[~occ,1],s=6,c=B,label="detections"); a.plot(est[:,0],est[:,1],color=R,lw=1.5,label="Kalman"); a.plot(est[occ,0],est[occ,1],color=O,lw=3,label="predicted only (occluded)"); a.legend(fontsize=7); a.set_title("Track survives a 25-frame occlusion",fontsize=9)
a=axs[1]; a.plot(np.arange(T),sd,color=P,lw=2); a.axvspan(50,75,color="#fed7d7",alpha=.6); a.set_xlabel("frame"); a.set_ylabel("position σ (px)"); a.set_title("Uncertainty grows while blind, collapses on re-detection",fontsize=9)
a=axs[2]; a.plot([r[0] for r in rows],[r[2] for r in rows],"o-",color=B,label="hold last position"); a.plot([r[0] for r in rows],[r[1] for r in rows],"o-",color=G,label="Kalman (smooth motion)"); a.plot([r[0] for r in rows2],[r[2] for r in rows2],"s--",color=R,label="hold, accelerating object"); a.plot([r[0] for r in rows2],[r[1] for r in rows2],"s--",color=O,label="Kalman, accelerating object"); a.set_xlabel("occlusion length (frames)"); a.set_ylabel("mean error during occlusion (px)"); a.legend(fontsize=7); a.set_title("Prediction beats 'hold last position' at every length;\nboth degrade as the blind period grows",fontsize=9); a.grid(alpha=.3)
plt.tight_layout(); save("cv_07_tracking_occlusion.png")

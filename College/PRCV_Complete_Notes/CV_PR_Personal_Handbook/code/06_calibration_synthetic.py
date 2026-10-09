"""Camera calibration lab with ground truth: how many views, how much tilt, how much noise? (Davies ch.18; Baggio ch.2,4)."""
from _common import *
import cv2
Ktrue=np.array([[800,0,320],[0,800,240],[0,0,1.]]); size=(640,480)
obj=np.zeros((9*6,3),np.float32); obj[:,:2]=np.mgrid[0:9,0:6].T.reshape(-1,2)*0.025     # 9x6 corners, 25 mm squares
def make_views(n,tilt_deg,noise,rng):
    O,I=[],[]
    while len(O)<n:
        rv=np.radians(rng.uniform(-tilt_deg,tilt_deg,3)); tv=np.array([rng.uniform(-.08,.08),rng.uniform(-.06,.06),rng.uniform(.45,.9)])
        pts,_=cv2.projectPoints(obj,rv,tv,Ktrue,None); pts=pts.reshape(-1,2)
        if (pts[:,0]>5).all() and (pts[:,0]<size[0]-5).all() and (pts[:,1]>5).all() and (pts[:,1]<size[1]-5).all():
            O.append(obj); I.append((pts+rng.randn(*pts.shape)*noise).astype(np.float32))
    return O,I
def trial(n,tilt,noise,seed):
    rng=np.random.RandomState(seed); O,I=make_views(n,tilt,noise,rng)
    flags=cv2.CALIB_FIX_ASPECT_RATIO|cv2.CALIB_ZERO_TANGENT_DIST|cv2.CALIB_FIX_K1|cv2.CALIB_FIX_K2|cv2.CALIB_FIX_K3
    K0=np.array([[700,0,size[0]/2],[0,700,size[1]/2],[0,0,1.]])
    rms,K,dist,_,_=cv2.calibrateCamera(O,I,size,K0,None,flags=flags|cv2.CALIB_USE_INTRINSIC_GUESS); return K[0,0],K[0,2],K[1,2],rms
rows=[]
print("mean abs error of focal length fx (true 800 px) over 25 random trials")
print(f"{'views':>5s} {'tilt°':>5s} {'noise px':>8s} | fx error px | principal-point error px | RMS reproj")
for tilt,noise,n in [(5,0.5,10),(15,0.5,10),(30,0.5,10),(30,0.5,3),(30,0.5,5),(30,0.5,20),(30,0.1,10),(30,1.0,10),(30,2.0,10)]:
    fx=[];pp=[];rr=[]
    for s in range(25):
        f,cx,cy,r=trial(n,tilt,noise,s); fx.append(abs(f-800)); pp.append(np.hypot(cx-320,cy-240)); rr.append(r)
    rows.append((tilt,noise,n,np.mean(fx),np.mean(pp),np.mean(rr))); print(f"{n:5d} {tilt:5d} {noise:8.1f} | {np.mean(fx):10.2f}  | {np.mean(pp):10.2f}              | {np.mean(rr):.3f}")
fig,axs=plt.subplots(1,3,figsize=(14,3.6))
tl=[r for r in rows if r[1]==0.5 and r[2]==10]; axs[0].bar([str(r[0]) for r in tl],[r[3] for r in tl],color=B); axs[0].set_xlabel("max tilt of the board between views (deg)"); axs[0].set_ylabel("|fx error| (px)"); axs[0].set_title("Pose diversity matters most:\nnear-parallel views barely constrain f",fontsize=9)
nv=[r for r in rows if r[0]==30 and r[1]==0.5]; nv.sort(key=lambda r:r[2]); axs[1].plot([r[2] for r in nv],[r[3] for r in nv],"o-",color=G); axs[1].set_xlabel("number of views"); axs[1].set_ylabel("|fx error| (px)"); axs[1].set_title("More views help, with diminishing returns",fontsize=9); axs[1].grid(alpha=.3)
ns=[r for r in rows if r[0]==30 and r[2]==10]; ns.sort(key=lambda r:r[1]); axs[2].plot([r[1] for r in ns],[r[3] for r in ns],"o-",color=R,label="|fx error|"); axs[2].plot([r[1] for r in ns],[r[5]*10 for r in ns],"s--",color=GR,label="RMS reprojection ×10"); axs[2].set_xlabel("corner-detection noise σ (px)"); axs[2].legend(fontsize=8); axs[2].set_title("Error grows ~linearly with noise;\nRMS reprojection ≈ noise level (not accuracy!)",fontsize=9); axs[2].grid(alpha=.3)
plt.tight_layout(); save("cv_04_calibration.png")

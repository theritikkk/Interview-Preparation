"""Marker-based AR core (Baggio ch.2): detect a square marker, recover 3-D pose with PnP, compare with ground truth."""
from _common import *
import cv2
K=np.array([[900,0,320],[0,900,240],[0,0,1.]]); size=(640,480); L=0.10   # marker side 10 cm
dic=cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_4X4_50); det=cv2.aruco.ArucoDetector(dic,cv2.aruco.DetectorParameters())
mk=cv2.aruco.generateImageMarker(dic,7,200); mk=cv2.copyMakeBorder(mk,25,25,25,25,cv2.BORDER_CONSTANT,value=255)   # white quiet zone
obj=np.float32([[-L/2,L/2,0],[L/2,L/2,0],[L/2,-L/2,0],[-L/2,-L/2,0]])
src=np.float32([[25,25],[224,25],[224,224],[25,224]])        # marker corners inside the bordered image
rng=np.random.RandomState(0)
def render(rv,tv,blur=0,noise=0,gain=1.0):
    pts,_=cv2.projectPoints(obj,rv,tv,K,None); Hm=cv2.getPerspectiveTransform(src,pts.reshape(-1,2).astype(np.float32))
    bg=np.clip(rng.randn(*size[::-1])*12+120,0,255).astype(np.uint8); w=cv2.warpPerspective(mk,Hm,size,flags=cv2.INTER_LINEAR); m=cv2.warpPerspective(np.full(mk.shape,255,np.uint8),Hm,size)>128
    im=bg.copy(); im[m]=w[m]; im=(im*gain).astype(float)
    if blur: im=cv2.GaussianBlur(im,(0,0),blur)
    return np.clip(im+rng.randn(*im.shape)*noise,0,255).astype(np.uint8),pts.reshape(-1,2)
def trial(angle,dist,**kw):
    ax=rng.randn(3); ax/=np.linalg.norm(ax); rv0=np.array([np.pi,0,0.])      # marker facing the camera (flip about x), then tilt
    R0,_=cv2.Rodrigues(rv0); Rt,_=cv2.Rodrigues(ax*np.radians(angle)); rv,_=cv2.Rodrigues(Rt@R0); tv=np.array([rng.uniform(-.05,.05),rng.uniform(-.05,.05),dist])
    im,_=render(rv,tv,**kw); c,ids,_=det.detectMarkers(im)
    if ids is None or 7 not in ids.ravel(): return None
    cc=c[list(ids.ravel()).index(7)].reshape(4,2)
    ok,rvec,tvec=cv2.solvePnP(obj,cc,K,None,flags=cv2.SOLVEPNP_IPPE_SQUARE)
    if not ok: return None
    Rg,_=cv2.Rodrigues(rv); Re,_=cv2.Rodrigues(rvec); dR=np.degrees(np.arccos(np.clip((np.trace(Rg.T@Re)-1)/2,-1,1)))
    return np.linalg.norm(tvec.ravel()-tv)*1000, dR, abs(tvec[2,0]-dist)*1000
print(f"{'condition':30s} {'detected':>8s} {'pos err mm':>10s} {'rot err deg':>11s} {'depth err mm':>12s}")
rows=[]
for name,angle,dist,kw in [("frontal, 0.5 m, clean",5,0.5,{}),("frontal, 1.0 m",5,1.0,{}),("frontal, 2.0 m",5,2.0,{}),("frontal, 3.0 m",5,3.0,{}),("tilt 45°, 0.5 m",45,0.5,{}),("tilt 70°, 0.5 m",70,0.5,{}),("tilt 80°, 0.5 m",80,0.5,{}),("0.5 m, blur σ=2",5,0.5,dict(blur=2)),("0.5 m, blur σ=4",5,0.5,dict(blur=4)),("0.5 m, noise σ=25",5,0.5,dict(noise=25)),("0.5 m, dark (gain 0.3)+noise 10",5,0.5,dict(gain=0.3,noise=10))]:
    res=[trial(angle,dist,**kw) for _ in range(40)]; ok=[r for r in res if r]; 
    d=len(ok)/40; e=np.mean([r[0] for r in ok]) if ok else np.nan; ro=np.mean([r[1] for r in ok]) if ok else np.nan; de=np.mean([r[2] for r in ok]) if ok else np.nan
    rows.append((name,d,e,ro,de)); print(f"{name:30s} {d:8.2f} {e:10.1f} {ro:11.2f} {de:12.1f}")
# stages figure
rv0=np.array([np.pi,0,0.]); R0,_=cv2.Rodrigues(rv0); Rt,_=cv2.Rodrigues(np.array([0.5,0.4,0.1])); rv,_=cv2.Rodrigues(Rt@R0); tv=np.array([0.02,-0.01,0.6]); im,_=render(rv,tv,blur=1,noise=6)
c,ids,_=det.detectMarkers(im); vis=cv2.cvtColor(im,cv2.COLOR_GRAY2BGR); cv2.aruco.drawDetectedMarkers(vis,c,ids); cc=c[0].reshape(4,2); ok,rvec,tvec=cv2.solvePnP(obj,cc,K,None,flags=cv2.SOLVEPNP_IPPE_SQUARE); cv2.drawFrameAxes(vis,K,None,rvec,tvec,0.07,3)
# virtual cube
cube=np.float32([[-L/2,-L/2,0],[L/2,-L/2,0],[L/2,L/2,0],[-L/2,L/2,0],[-L/2,-L/2,L],[L/2,-L/2,L],[L/2,L/2,L],[-L/2,L/2,L]]); pp,_=cv2.projectPoints(cube,rvec,tvec,K,None); pp=pp.reshape(-1,2).astype(int)
for a_,b_ in [(0,1),(1,2),(2,3),(3,0),(4,5),(5,6),(6,7),(7,4),(0,4),(1,5),(2,6),(3,7)]: cv2.line(vis,tuple(pp[a_]),tuple(pp[b_]),(0,200,255),2)
fig,axs=plt.subplots(1,3,figsize=(15,3.9)); axs[0].imshow(im,cmap="gray"); axs[0].set_title("camera frame (tilted, blurred, noisy marker)",fontsize=9); axs[0].axis("off")
axs[1].imshow(vis[...,::-1]); axs[1].set_title("detect 4 corners → solvePnP → axes + virtual cube\n(= marker-based AR: pose gives the 3-D → 2-D mapping)",fontsize=9); axs[1].axis("off")
nm=[r[0] for r in rows]; axs[2].barh(nm[::-1],[r[1] for r in rows][::-1],color=[G if r[1]>.9 else (O if r[1]>.4 else R) for r in rows][::-1]); axs[2].set_xlim(0,1.05); axs[2].set_title("detection rate (40 trials each)",fontsize=9); axs[2].tick_params(axis="y",labelsize=7)
plt.tight_layout(); save("cv_05_aruco_pose.png")
fig,ax=plt.subplots(1,2,figsize=(10,3.4)); dd=[r for r in rows if "frontal" in r[0]]; ax[0].plot([0.5,1,2,3],[r[4] for r in dd],"o-",color=R); ax[0].set_xlabel("distance (m)"); ax[0].set_ylabel("depth error (mm)"); ax[0].set_title("Depth error grows rapidly with distance\n(fewer pixels on the marker)",fontsize=9); ax[0].grid(alpha=.3)
tt=[r for r in rows if "tilt" in r[0] or r[0].startswith("frontal, 0.5")]; ax[1].plot([5,45,70,80],[r[3] for r in tt],"o-",color=B); ax[1].set_xlabel("marker tilt (deg)"); ax[1].set_ylabel("rotation error (deg)"); ax[1].set_title("Rotation error vs tilt",fontsize=9); ax[1].grid(alpha=.3)
plt.tight_layout(); save("cv_06_aruco_errors.png")

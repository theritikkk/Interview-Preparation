"""Feature matching + RANSAC homography: the engine behind marker-less AR, stitching, SfM (Baggio ch.3-4; Davies ch.6,18)."""
from _common import *
import cv2
from skimage import data
img=cv2.resize(data.camera(),(480,480)); H=np.array([[0.88,-0.22,70],[0.20,0.90,35],[2e-4,-1e-4,1.0]])
warp=cv2.warpPerspective(img,H,(560,560)); rng=np.random.RandomState(0)
def corrupt(im,noise,gain): return np.clip(im.astype(float)*gain+rng.randn(*im.shape)*noise,0,255).astype(np.uint8)
corners=np.float32([[0,0],[479,0],[479,479],[0,479]]).reshape(-1,1,2); truth=cv2.perspectiveTransform(corners,H)
def run(det,name,noise,gain,ratio=True):
    w=corrupt(warp,noise,gain); k1,d1=det.detectAndCompute(img,None); k2,d2=det.detectAndCompute(w,None)
    norm=cv2.NORM_HAMMING if name=="ORB" else cv2.NORM_L2; bf=cv2.BFMatcher(norm)
    m=[a for a,b in bf.knnMatch(d1,d2,k=2) if a.distance<0.75*b.distance]
    if len(m)<8: return None
    p1=np.float32([k1[a.queryIdx].pt for a in m]).reshape(-1,1,2); p2=np.float32([k2[a.trainIdx].pt for a in m]).reshape(-1,1,2)
    out={}
    for meth,lab in [(cv2.RANSAC,"RANSAC"),(0,"least-squares (all matches)")]:
        Hh,mask=cv2.findHomography(p1,p2,meth,3.0)
        if Hh is None: out[lab]=np.nan; continue
        err=np.linalg.norm(cv2.perspectiveTransform(corners,Hh)-truth,axis=2).mean(); out[lab]=err
        if meth==cv2.RANSAC: inl=int(mask.sum())
    return len(k1),len(m),inl,out,(k1,k2,m,mask,w)
orb=cv2.ORB_create(1500); sift=cv2.SIFT_create(1500)
print(f"{'detector':6s} {'noise σ':>7s} {'gain':>5s} | matches inliers | corner error (px): RANSAC vs LS-all-matches")
keep=None
for det,name in [(orb,"ORB"),(sift,"SIFT")]:
    for noise,gain in [(0,1.0),(15,1.0),(15,0.6),(35,0.6)]:
        r=run(det,name,noise,gain)
        if r is None: print(f"{name:6s} {noise:7d} {gain:5.1f} | too few matches"); continue
        nk,nm,ni,e,pack=r; print(f"{name:6s} {noise:7d} {gain:5.1f} | {nm:4d}    {ni:4d}   | {e['RANSAC']:.2f} vs {e['least-squares (all matches)']:.1f}")
        if name=="SIFT" and noise==15 and gain==0.6: keep=pack
k1,k2,m,mask,w=keep
vis=cv2.drawMatches(img,k1,w,k2,[a for a,f in zip(m,mask.ravel()) if f][:60],None,matchColor=(0,200,0),flags=cv2.DrawMatchesFlags_NOT_DRAW_SINGLE_POINTS)
fig,ax=plt.subplots(figsize=(13,5.6)); ax.imshow(vis[...,::-1] if vis.ndim==3 else vis); ax.axis("off"); ax.set_title("SIFT matches kept by RANSAC (green) between an image and a perspective-warped, darker, noisy copy",fontsize=10); save("cv_03_matching_ransac.png")

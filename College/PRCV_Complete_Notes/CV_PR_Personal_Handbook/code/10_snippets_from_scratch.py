"""Interview-grade building blocks implemented from scratch and CHECKED against reference libraries.
Run: python 10_snippets_from_scratch.py   (prints PASS lines; raises if anything disagrees)"""
import numpy as np, cv2
rng=np.random.RandomState(0)

# 1. 2-D convolution / correlation (valid, zero-pad 'same') vs OpenCV
def correlate2d(img,k):
    kh,kw=k.shape; ph,pw=kh//2,kw//2; p=np.pad(img.astype(float),((ph,ph),(pw,pw))); out=np.zeros(img.shape,float)
    for dy in range(kh):
        for dx in range(kw): out+=k[dy,dx]*p[dy:dy+img.shape[0],dx:dx+img.shape[1]]
    return out
img=rng.randint(0,256,(40,50)).astype(np.float32); k=rng.randn(5,5).astype(np.float32)
assert np.allclose(correlate2d(img,k),cv2.filter2D(img,-1,k,borderType=cv2.BORDER_CONSTANT),atol=1e-2); print("PASS correlation matches cv2.filter2D")

# 2. Separable Gaussian = same result, far fewer multiplications: O(k) per pixel instead of O(k^2)
def gauss1d(s,r=None):
    r=r or int(3*s); x=np.arange(-r,r+1); g=np.exp(-x**2/(2*s*s)); return g/g.sum()
g=gauss1d(2.0); full=np.outer(g,g); a=correlate2d(img,full); b=correlate2d(correlate2d(img,g[None,:]),g[:,None]); assert np.allclose(a,b,atol=1e-6); print(f"PASS separable Gaussian: {full.size} mults/pixel -> {2*g.size} (kernel {g.size}x{g.size})")

# 3. Integral image: any box sum in O(1)
ii=np.pad(img.astype(np.float64),((1,0),(1,0))).cumsum(0).cumsum(1)
def boxsum(y0,x0,y1,x1): return ii[y1,x1]-ii[y0,x1]-ii[y1,x0]+ii[y0,x0]
assert np.isclose(boxsum(5,7,25,30),img[5:25,7:30].sum()); print("PASS integral image box sum")

# 4. Otsu from scratch vs OpenCV
def otsu(gray):
    h=np.bincount(gray.ravel(),minlength=256)/gray.size; w0=np.cumsum(h); mu=np.cumsum(h*np.arange(256)); mt=mu[-1]
    with np.errstate(divide="ignore",invalid="ignore"): sb=(mt*w0-mu)**2/(w0*(1-w0))
    return int(np.nanargmax(sb))
from skimage import data
g8=data.coins(); t_mine=otsu(g8); t_cv,_=cv2.threshold(g8,0,255,cv2.THRESH_BINARY+cv2.THRESH_OTSU); assert abs(t_mine-t_cv)<=1; print(f"PASS Otsu: mine={t_mine}, OpenCV={t_cv:.0f}")

# 5. Harris response from scratch vs cv2.cornerHarris (same window handling up to normalisation)
def harris(gray,k=0.04,sigma=1.5):
    g=gray.astype(np.float64); Ix=cv2.Sobel(g,cv2.CV_64F,1,0,ksize=3); Iy=cv2.Sobel(g,cv2.CV_64F,0,1,ksize=3)
    Sxx=cv2.GaussianBlur(Ix*Ix,(0,0),sigma); Syy=cv2.GaussianBlur(Iy*Iy,(0,0),sigma); Sxy=cv2.GaussianBlur(Ix*Iy,(0,0),sigma)
    return (Sxx*Syy-Sxy**2)-k*(Sxx+Syy)**2
sq=np.zeros((80,80),np.uint8); sq[20:60,20:60]=255; R=harris(sq); ys,xs=np.unravel_index(np.argsort(R.ravel())[-4:],R.shape)
print("PASS Harris: strongest 4 responses at",sorted(zip(ys.tolist(),xs.tolist())),"(square corners at (20,20),(20,59),(59,20),(59,59))")

# 6. IoU and non-maximum suppression (detectors, every interview)
def iou(a,b):
    x0,y0=max(a[0],b[0]),max(a[1],b[1]); x1,y1=min(a[2],b[2]),min(a[3],b[3]); inter=max(0,x1-x0)*max(0,y1-y0)
    return inter/((a[2]-a[0])*(a[3]-a[1])+(b[2]-b[0])*(b[3]-b[1])-inter+1e-12)
def nms(boxes,scores,thr=0.5):
    order=np.argsort(scores)[::-1]; keep=[]
    while len(order):
        i=order[0]; keep.append(int(i)); order=np.array([j for j in order[1:] if iou(boxes[i],boxes[j])<=thr],dtype=int)
    return keep
boxes=[(10,10,60,60),(12,12,62,62),(100,100,150,150),(11,9,58,61)]; sc=[0.9,0.8,0.7,0.95]
assert nms(boxes,sc)==[3,2]; assert abs(iou((0,0,10,10),(5,5,15,15))-25/175)<1e-9; print("PASS IoU / NMS")

# 7. Average precision (area under PR curve, all-point interpolation) vs sklearn
from sklearn.metrics import average_precision_score
def ap(y,s):
    o=np.argsort(-s); y=y[o]; tp=np.cumsum(y); fp=np.cumsum(1-y); rec=tp/y.sum(); prec=tp/(tp+fp)
    return float(np.sum(np.diff(np.r_[0,rec])*prec))
y=rng.randint(0,2,500); s=y*0.8+rng.randn(500)*0.7; assert abs(ap(y,s)-average_precision_score(y,s))<1e-9; print("PASS average precision")

# 8. RANSAC line fit from scratch
def ransac_line(p,iters=200,thr=2.0):
    best=(0,None)
    for _ in range(iters):
        i,j=rng.choice(len(p),2,replace=False); d=p[j]-p[i]; n=np.array([-d[1],d[0]]); n=n/ (np.linalg.norm(n)+1e-12); dist=np.abs((p-p[i])@n); c=(dist<thr).sum()
        if c>best[0]: best=(c,(p[i],n,dist<thr))
    return best[1]
xs_=np.linspace(0,100,100); pts=np.c_[xs_,0.5*xs_+10+rng.randn(100)]
out=rng.rand(100)<0.4; pts[out,1]=rng.uniform(-30,130,out.sum()); p0,n,inl=ransac_line(pts); q=pts[inl]; sl=np.polyfit(q[:,0],q[:,1],1)[0]; assert abs(sl-0.5)<0.03; print(f"PASS RANSAC slope={sl:.3f} with 40% outliers")

# 9. DLT homography from 4+ points vs OpenCV
src=np.float32([[0,0],[100,0],[100,100],[0,100],[50,50]]); Ht=np.array([[1.1,0.2,10],[-0.1,0.9,20],[1e-3,2e-3,1]]); 
dst=cv2.perspectiveTransform(src[None],Ht)[0]
def dlt(src,dst):
    A=[]
    for (x,y),(u,v) in zip(src,dst): A+= [[-x,-y,-1,0,0,0,u*x,u*y,u],[0,0,0,-x,-y,-1,v*x,v*y,v]]
    _,_,Vt=np.linalg.svd(np.array(A)); H=Vt[-1].reshape(3,3); return H/H[2,2]
assert np.allclose(dlt(src,dst),Ht/Ht[2,2],atol=1e-6); print("PASS DLT homography (SVD of the 2N x 9 system) recovers H")

# 10. k-means (Lloyd) from scratch vs sklearn inertia
from sklearn.cluster import KMeans
X=np.vstack([rng.randn(100,2)+c for c in ([0,0],[6,0],[3,5])])
def kmeans(X,k,it=50):
    C=X[rng.choice(len(X),k,replace=False)]
    for _ in range(it):
        lab=((X[:,None]-C[None])**2).sum(2).argmin(1); Cn=np.array([X[lab==j].mean(0) if (lab==j).any() else C[j] for j in range(k)])
        if np.allclose(Cn,C): break
        C=Cn
    return lab,C,((X-C[lab])**2).sum()
best=min(kmeans(X,3)[2] for _ in range(5)); ref=KMeans(3,n_init=5,random_state=0).fit(X).inertia_; assert abs(best-ref)/ref<0.02; print(f"PASS k-means inertia mine={best:.1f} sklearn={ref:.1f}")

# 11. PCA via SVD vs sklearn; LDA direction (two-class Fisher) closed form
from sklearn.decomposition import PCA
Z=rng.randn(200,5)@rng.randn(5,5); U,S,Vt=np.linalg.svd(Z-Z.mean(0),full_matrices=False); p=PCA(2).fit(Z); assert np.allclose(np.abs(Vt[:2]),np.abs(p.components_),atol=1e-6); print("PASS PCA = SVD of the centred data")
A1=rng.randn(100,2)@[[2,1],[0,1]]+[0,0]; A2=rng.randn(100,2)@[[2,1],[0,1]]+[3,2]; Sw=np.cov(A1.T)*99+np.cov(A2.T)*99; w=np.linalg.solve(Sw,A2.mean(0)-A1.mean(0)); 
from sklearn.discriminant_analysis import LinearDiscriminantAnalysis as LDA
wl=LDA(solver="eigen").fit(np.vstack([A1,A2]),[0]*100+[1]*100).scalings_[:,0]; cosang=abs(w@wl)/np.linalg.norm(w)/np.linalg.norm(wl); assert cosang>0.9999; print("PASS Fisher direction w ∝ Sw^-1 (m2-m1) equals sklearn LDA direction")

# 12. Camera projection and triangulation round trip (linear DLT triangulation)
K=np.array([[800,0,320],[0,800,240],[0,0,1.]]); R1,t1=np.eye(3),np.zeros(3); th=np.radians(10); R2=np.array([[np.cos(th),0,np.sin(th)],[0,1,0],[-np.sin(th),0,np.cos(th)]]); t2=np.array([-0.5,0,0.05])
P1=K@np.c_[R1,t1]; P2=K@np.c_[R2,t2]; X3=np.array([0.3,-0.2,4.0,1.0]); x1=P1@X3; x1=x1[:2]/x1[2]; x2=P2@X3; x2=x2[:2]/x2[2]
A=np.array([x1[0]*P1[2]-P1[0],x1[1]*P1[2]-P1[1],x2[0]*P2[2]-P2[0],x2[1]*P2[2]-P2[1]]); _,_,Vt=np.linalg.svd(A); Xr=Vt[-1]; Xr=Xr/Xr[3]; assert np.allclose(Xr[:3],X3[:3],atol=1e-6); print("PASS triangulation recovers the 3-D point:",np.round(Xr[:3],4))
print("ALL PASS")

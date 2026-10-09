"""Hough transform & RANSAC: robust to clutter and occlusion (Davies ch.11-13)."""
from _common import *
import cv2
from skimage.transform import hough_line, hough_line_peaks
from skimage.measure import ransac, LineModelND
rng=np.random.RandomState(1)
img=np.zeros((300,420),np.uint8); cv2.line(img,(20,40),(400,60),255,2); cv2.line(img,(60,280),(380,120),255,2); cv2.line(img,(210,10),(180,290),255,2); cv2.circle(img,(300,200),55,255,2)
noisy=np.clip(img+rng.randn(*img.shape)*18+rng.rand(*img.shape)*0,0,255).astype(np.uint8); ed=cv2.Canny(cv2.GaussianBlur(noisy,(5,5),1.2),60,140)
h,th,d=hough_line(ed>0); _,ang,dist=hough_line_peaks(h,th,d,num_peaks=3,min_distance=15,min_angle=5,threshold=0.25*h.max())
print("Hough lines (angle deg, rho):",[(round(float(np.degrees(a)),1),round(float(r),1)) for a,r in zip(ang,dist)])
# circle detection vs occlusion fraction
res=[]
for occ in [0,0.25,0.5,0.65,0.8]:
    ok=0; errs=[]
    for t in range(20):
        im=np.zeros((200,200),np.uint8); a0=rng.uniform(0,360); sweep=360*(1-occ); cv2.ellipse(im,(100,100),(50,50),0,a0,a0+sweep,255,2)
        im=np.clip(im+rng.randn(*im.shape)*10+(rng.rand(*im.shape)<0.02)*200,0,255).astype(np.uint8)
        c=cv2.HoughCircles(cv2.GaussianBlur(im,(5,5),1.2),cv2.HOUGH_GRADIENT,dp=1,minDist=50,param1=100,param2=max(6,int(25*(1-occ))+4),minRadius=40,maxRadius=60)
        if c is not None:
            x,y,r=c[0][0]
            if np.hypot(x-100,y-100)<4 and abs(r-50)<4: ok+=1; errs.append(np.hypot(x-100,y-100))
    res.append((occ,ok/20,np.mean(errs) if errs else np.nan))
print("circle detection success vs occlusion (20 trials each; param2 adapted):",[(o,s) for o,s,_ in res])
# RANSAC vs least squares with outliers
n=100; xs=np.linspace(0,100,n); ys=0.5*xs+10+rng.randn(n)*1.0; out=rng.rand(n)<0.4; ys[out]=rng.uniform(-30,130,out.sum())
pts=np.c_[xs,ys]; A=np.c_[xs,np.ones(n)]; ls=np.linalg.lstsq(A,ys,rcond=None)[0]
model,inl=ransac(pts,LineModelND,min_samples=2,residual_threshold=3,max_trials=200,rng=0)
o=model.params[0]; dvec=model.params[1]; slope=dvec[1]/dvec[0]; icpt=o[1]-slope*o[0]
print(f"40% outliers: least squares slope={ls[0]:.3f} intercept={ls[1]:.1f}; RANSAC slope={slope:.3f} intercept={icpt:.1f}; truth 0.5, 10")
fig,axs=plt.subplots(1,4,figsize=(16,3.8))
axs[0].imshow(noisy,cmap="gray"); axs[0].set_title("noisy input",fontsize=9)
for a,r in zip(ang,dist):
    x0=np.array([0,noisy.shape[1]]); y0=(r-x0*np.cos(a))/np.sin(a); axs[0].plot(x0,y0,"-r",lw=1.2)
axs[0].set_xlim(0,noisy.shape[1]); axs[0].set_ylim(noisy.shape[0],0)
axs[1].imshow(np.log1p(h),extent=[np.degrees(th[-1]),np.degrees(th[0]),d[-1],d[0]],aspect="auto",cmap="magma"); axs[1].set_xlabel("θ (deg)"); axs[1].set_ylabel("ρ"); axs[1].set_title("Hough accumulator: each line = a bright peak\n(votes survive noise & gaps)",fontsize=9)
axs[2].plot([o*100 for o,_,_ in res],[s*100 for _,s,_ in res],"o-",color=B); axs[2].set_xlabel("% of the circle occluded"); axs[2].set_ylabel("detection success (%)"); axs[2].set_title("Hough circles: graceful degradation\nwith occlusion",fontsize=9); axs[2].grid(alpha=.3); axs[2].set_ylim(-5,105)
axs[3].scatter(xs[~inl],ys[~inl],c=GR,s=14,label="outliers"); axs[3].scatter(xs[inl],ys[inl],c=B,s=14,label="RANSAC inliers"); xx=np.array([0,100]); axs[3].plot(xx,ls[0]*xx+ls[1],"--",color=R,label="least squares"); axs[3].plot(xx,slope*xx+icpt,color=G,lw=2,label="RANSAC"); axs[3].legend(fontsize=7); axs[3].set_title("Robust fitting: 40% outliers",fontsize=9)
plt.tight_layout(); save("cv_02_hough_ransac.png")

"""Classical measurement pipeline: threshold -> clean -> split touching objects -> measure (Davies ch.4,7,9; Baggio ch.5 style)."""
from _common import *
import cv2
from scipy import ndimage as ndi
from skimage import data, filters, measure, morphology, segmentation, feature
img=data.coins(); T=filters.threshold_otsu(img)
def run(bw,md=16):
    bw=ndi.binary_fill_holes(morphology.opening(bw,morphology.disk(3))); bw=morphology.remove_small_objects(bw,150)
    lab0,n0=ndi.label(bw); edt=ndi.distance_transform_edt(bw); pk=feature.peak_local_max(ndi.gaussian_filter(edt,2),min_distance=md,labels=lab0)
    mk=np.zeros_like(lab0); mk[tuple(pk.T)]=np.arange(1,len(pk)+1); ws=segmentation.watershed(-edt,mk,mask=bw); return bw,lab0,n0,edt,pk,ws
for name,bw_ in [("global Otsu",img>T),("local (block 101)",img>filters.threshold_local(img,101))]:
    bw,lab0,n0,edt,pk,ws=run(bw_); pr=[p for p in measure.regionprops(ws) if p.area>300]; d=np.array([p.equivalent_diameter_area for p in pr])
    print(f"{name:18s}: components={n0:2d}, objects after watershed={len(pr):2d} (ground truth 24), diameter mean={d.mean():.1f}px std={d.std():.1f}, largest={d.max():.0f}px")
bw,lab0,n0,edt,pk,ws=run(img>filters.threshold_local(img,101)); props=[p for p in measure.regionprops(ws) if p.area>300]
d=np.array([p.equivalent_diameter_area for p in props]); print("flagged solidity<0.93:",sum(p.solidity<0.93 for p in props),"of",len(props))
fig,axs=plt.subplots(1,4,figsize=(15,3.6))
axs[0].imshow(img,cmap="gray"); axs[0].set_title("input",fontsize=9)
axs[1].imshow(bw,cmap="gray"); axs[1].set_title(f"local threshold + opening + fill\n{n0} connected components",fontsize=9)
axs[2].imshow(edt,cmap="viridis"); axs[2].scatter(pk[:,1],pk[:,0],c="r",s=8); axs[2].set_title("distance transform + peaks\n(markers for watershed)",fontsize=9)
ov=segmentation.mark_boundaries(np.dstack([img]*3)/255.,ws,color=(1,0,0)); axs[3].imshow(ov); axs[3].set_title(f"watershed split: {len(props)} objects\n(each can now be measured)",fontsize=9)
for p in props:
    if True: axs[3].text(p.centroid[1],p.centroid[0],f"{p.equivalent_diameter_area:.0f}",color="yellow",fontsize=6,ha="center")
for a in axs: a.axis("off")
plt.tight_layout(); save("cv_01_segment_measure.png")

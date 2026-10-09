from _common import *
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch, Circle, Rectangle, Polygon, Ellipse
def box(ax,x,y,w,h,t,fc="#ebf4ff",ec=B,fs=9,bold=False):
    ax.add_patch(FancyBboxPatch((x-w/2,y-h/2),w,h,boxstyle="round,pad=0.02,rounding_size=0.08",fc=fc,ec=ec,lw=1.4)); ax.text(x,y,t,ha="center",va="center",fontsize=fs,fontweight="bold" if bold else "normal")
def arr(ax,p,q,c="#4a5568",lw=1.4,rad=0,ls="-",st="-|>"): ax.add_patch(FancyArrowPatch(p,q,arrowstyle=st,mutation_scale=13,color=c,lw=lw,connectionstyle=f"arc3,rad={rad}",linestyle=ls))
def blank(w,h,xl,yl):
    fig,ax=plt.subplots(figsize=(w,h)); ax.set_xlim(0,xl); ax.set_ylim(0,yl); ax.axis("off"); return fig,ax

# D1 big picture
fig,ax=blank(13.6,6.4,13.6,6.4)
ax.text(6.8,6.15,"Vision & pattern recognition: one pipeline, two eras",ha="center",fontsize=13,fontweight="bold")
stages=[("World +\nlighting","#fefcbf",O),("Sensor\n(camera, depth,\nX-ray…)","#fefcbf",O),("Pre-processing\n(denoise, normalise,\ngeometry)","#e6fffa",G),("Representation\n(features)","#ebf4ff",B),("Decision\n(classifier /\nestimator)","#fed7e2",R),("Action\n(reject, steer,\nalert, report)","#e9d8fd",P)]
for i,(t,c,e) in enumerate(stages):
    x=1.1+i*2.2; box(ax,x,4.6,1.9,1.25,t,fc=c,ec=e,fs=9)
    if i<5: arr(ax,(x+.98,4.6),(x+1.22,4.6))
ax.text(0.2,3.55,"CLASSICAL (Davies, Duda, Beyerer, Baggio)",fontsize=10,fontweight="bold",color=B)
ax.text(0.2,3.05,"Engineer controls lighting & optics → hand-designed features (edges, corners, SIFT/HOG, moments, Fourier descriptors)\n→ statistical classifier (Bayes, k-NN, SVM, trees) → explicit geometry (calibration, PnP, epipolar) for 3-D",fontsize=9,va="top")
ax.text(0.2,1.95,"DEEP-LEARNING ERA",fontsize=10,fontweight="bold",color=R)
ax.text(0.2,1.45,"Features and classifier are learned jointly from data (CNN / Transformer) → data, labels, augmentation and compute replace hand design;\ngeometry, calibration, lighting, evaluation, decision theory and failure analysis are STILL needed",fontsize=9,va="top")
ax.text(6.8,0.35,"What does not go away: problem definition · data quality · evaluation · cost of errors · deployment constraints",ha="center",fontsize=9.5,fontweight="bold",color=G)
save("hb_01_big_picture.png")

# D2 method chooser
fig,ax=blank(14,7.4,14,7.4)
ax.text(7,7.15,"Which approach should I try first? (a practical chooser)",ha="center",fontsize=13,fontweight="bold")
box(ax,7,6.4,4.2,.7,"Can you CONTROL lighting & viewpoint?",fc="#fefcbf",ec=O,bold=True)
box(ax,3.1,5.0,4.8,.9,"YES → engineered vision\nbackground/contrast by design\n→ threshold + morphology + blob analysis",fc="#c6f6d5",ec=G,fs=8.5); box(ax,10.4,5.0,4.8,.9,"NO (natural scenes, appearance varies)\n→ learned features are usually needed",fc="#fed7e2",ec=R,fs=8.5)
arr(ax,(5.6,6.1),(3.6,5.5)); arr(ax,(8.4,6.1),(10.4,5.5))
box(ax,3.1,3.55,4.8,.8,"Is the object rigid with known geometry?\n→ model-based: Hough / PnP / template / ICP",fc="#ebf4ff",fs=8.5); arr(ax,(3.1,4.55),(3.1,3.95))
box(ax,10.4,3.55,4.8,.8,"Do you have labelled data (≥ 1000s of examples)?",fc="#fefcbf",ec=O,fs=8.5); arr(ax,(10.4,4.55),(10.4,3.95))
box(ax,7.9,2.05,3.4,1.05,"YES\n→ fine-tune a pretrained CNN/ViT\n(detector, segmenter, classifier)",fc="#e9d8fd",ec=P,fs=8.5); box(ax,11.5,2.05,3.4,1.05,"NO / very few\n→ classical features + SVM / trees,\nor pretrained embeddings + k-NN/LR,\naugmentation, synthetic data",fc="#e9d8fd",ec=P,fs=8.0)
arr(ax,(9.8,3.15),(8.2,2.6)); arr(ax,(11.0,3.15),(11.4,2.6))
box(ax,3.1,2.05,4.8,1.05,"Need 3-D / metric measurement?\ncalibrate camera (Davies 18, Baggio 2/4)\nstereo / structured light / depth sensor\n+ validate against a physical ruler",fc="#ebf4ff",fs=8.2); arr(ax,(3.1,3.15),(3.1,2.6))
box(ax,7,0.55,13.2,.7,"ALWAYS: define the cost of each error type → pick metrics (recall@precision, mAP, mm error) → hold out a realistic test set → baseline first → add complexity only if the baseline fails",fc="#e6fffa",ec=G,fs=8,bold=True)
save("hb_02_method_chooser.png")

# D3 pinhole + epipolar
fig,axs=plt.subplots(1,2,figsize=(14,5.2))
a=axs[0]; a.set_xlim(-2.5,10); a.set_ylim(-3.4,3.6); a.axis("off"); a.set_title("Pinhole camera: x ~ K [R | t] X",fontsize=11,fontweight="bold")
f=2.0; C=(0,0); Xp=(8,-2.2); a.plot([-f,-f],[-1.6,1.6],color=B,lw=4); a.text(-f,1.85,"image plane",ha="center",fontsize=9,color=B)
a.plot([-f-0.3,8.8],[0,0],"--",color=GR,lw=1); a.text(8.6,0.15,"optical axis",fontsize=8,color=GR,ha="right")
a.scatter(*C,c="k",s=50,zorder=5); a.text(0.15,0.25,"C (pinhole)",fontsize=9)
yi=-f*(-Xp[1])/Xp[0]*-1*-1; yi=f*Xp[1]/Xp[0]*-1   # similar triangles: image point is inverted
a.plot([Xp[0],-f],[Xp[1],yi],color=R,lw=1.6); a.scatter(*Xp,c=R,s=70); a.text(Xp[0]-.1,Xp[1]-.45,"scene point X",ha="right",fontsize=9,color=R); a.scatter(-f,yi,c=R,s=40,zorder=5); a.text(-f-.15,yi,"x",ha="right",va="center",fontsize=11,color=R)
a.annotate("",xy=(-f,-0.35),xytext=(0,-0.35),arrowprops=dict(arrowstyle="<->",color=G)); a.text(-f/2,-0.75,"f",ha="center",fontsize=11,color=G)
a.text(0.2,2.9,"K = [[fx, s, cx],[0, fy, cy],[0, 0, 1]]  intrinsics: how rays map to pixels\n[R | t]  extrinsics: where the camera is and how it points",fontsize=8.8,va="top")
a.text(3.9,-3.15,"ONE image loses depth: every point on the red ray gives the same pixel",ha="center",fontsize=9,color=R)
b=axs[1]; b.set_xlim(0,10); b.set_ylim(0,6); b.axis("off"); b.set_title("Two views: epipolar geometry (x'ᵀ F x = 0)",fontsize=11,fontweight="bold")
C1=(1.2,1.0); C2=(8.8,1.0); X=(5.2,5.0)
b.add_patch(Polygon([(0,2.0),(2.4,2.0),(2.4,2.0),(0,2.0)],fc="none")); b.plot([0.2,2.2],[2.3,1.7],color=B,lw=3); b.plot([7.8,9.8],[1.7,2.3],color=B,lw=3)
b.scatter(*C1,c="k"); b.scatter(*C2,c="k"); b.scatter(*X,c=R,s=70); b.text(X[0]+.15,X[1]+.1,"X",color=R,fontsize=11)
b.plot([C1[0],X[0]],[C1[1],X[1]],color=R,lw=1.2); b.plot([C2[0],X[0]],[C2[1],X[1]],color=R,lw=1.2); b.plot([C1[0],C2[0]],[C1[1],C2[1]],"--",color=GR)
x1=(1.2+0.45*(5.2-1.2)*.35, 0); 
# image points where rays intersect image segments (approximate by parametric)
def hit(C,Xp,p0,p1):
    d=np.array(Xp)-np.array(C); e=np.array(p1)-np.array(p0); M=np.array([d,-e]).T; s=np.linalg.solve(M,np.array(p0)-np.array(C)); return np.array(C)+s[0]*d
p1=hit(C1,X,(0.2,2.3),(2.2,1.7)); p2=hit(C2,X,(7.8,1.7),(9.8,2.3)); b.scatter(*p1,c=R,s=30); b.scatter(*p2,c=R,s=30); b.text(p1[0]-.5,p1[1]+.25,"x",color=R); b.text(p2[0]+.2,p2[1]+.25,"x′",color=R)
e1=hit(C1,C2,(0.2,2.3),(2.2,1.7)) if False else None
b.text(1.2,0.45,"camera 1",ha="center",fontsize=9); b.text(8.8,0.45,"camera 2",ha="center",fontsize=9); b.text(5.0,0.75,"baseline (t)",ha="center",fontsize=9,color=GR)
b.text(5.0,-0.35,"x′ must lie on the epipolar line F·x → matching becomes a 1-D search.\nF (or E = KᵀFK) from ≥ 8 (7, 5) matches + RANSAC → R, t → triangulate X → 3-D (Baggio ch.4).",ha="center",fontsize=8.5,va="top")
plt.tight_layout(); save("hb_03_camera_epipolar.png")

# D4 inspection station + lighting
fig,axs=plt.subplots(1,2,figsize=(15,5.2),gridspec_kw={"width_ratios":[1.1,1]})
a=axs[0]; a.set_xlim(0,10); a.set_ylim(0,6.4); a.axis("off"); a.set_title("An inspection station is a system, not an algorithm (Davies ch.20, 25-26)",fontsize=10,fontweight="bold")
a.add_patch(Rectangle((0.3,1.2),9.4,0.35,fc="#a0aec0")); a.text(5,0.8,"conveyor",ha="center",fontsize=8)
for x in (2.0,5.0,8.0): a.add_patch(Rectangle((x-.35,1.55),.7,.5,fc="#bee3f8",ec=B))
a.add_patch(Rectangle((4.2,4.7),1.6,.8,fc="#e2e8f0",ec="k")); a.text(5,5.1,"camera + lens",ha="center",fontsize=8.5); a.plot([4.5,3.4],[4.7,2.1],color=GR,lw=.8); a.plot([5.5,6.6],[4.7,2.1],color=GR,lw=.8)
a.add_patch(Rectangle((2.7,3.5),.5,.25,fc="gold",ec="k")); a.add_patch(Rectangle((6.8,3.5),.5,.25,fc="gold",ec="k")); a.text(2.95,3.9,"light",ha="center",fontsize=8); a.text(7.05,3.9,"light",ha="center",fontsize=8)
a.add_patch(Rectangle((1.6,2.7),0.1,0.7,fc=O)); a.text(1.0,3.1,"trigger\nsensor",fontsize=7.5,ha="center")
box(a,5,6.0,2.6,.5,"vision computer",fs=8.5,fc="#e9d8fd",ec=P); arr(a,(5,4.7),(5,5.75),c=P)
box(a,8.8,5.0,1.9,.9,"decision →\nreject / sort /\nPLC / log",fs=8,fc="#fed7e2",ec=R); arr(a,(6.3,6.0),(8.3,5.4),c=R); a.annotate("air-jet reject",(8.1,2.1),(8.8,3.7),arrowprops=dict(arrowstyle="->",color=R),fontsize=8,color=R,ha="center")
a.text(0.4,0.15,"Timing budget = belt speed × field of view ÷ processing time. Strobe to freeze motion; baseline every part with a golden sample.",fontsize=7.8)
b=axs[1]; b.set_xlim(0,10); b.set_ylim(0,6.4); b.axis("off"); b.set_title("Lighting is the cheapest 'algorithm' (Davies 25.2)",fontsize=10,fontweight="bold")
rows=[("Back-light","silhouette: crisp edges, gauging, presence","#c6f6d5"),("Diffuse / dome","uniform, hides texture & shiny glare","#bee3f8"),("Dark-field (low angle)","scratches, dust, engraved text glow on dark","#e9d8fd"),("Bright-field (coaxial)","flat mirror-like parts: defects look dark","#fefcbf"),("Structured light / laser line","3-D shape, height, volume","#fed7e2"),("Polarised / IR / UV","kill glare, see through film, fluoresce","#e2e8f0")]
for i,(n,d,c) in enumerate(rows):
    y=5.7-i*0.95; b.add_patch(FancyBboxPatch((0.2,y-.38),9.6,.76,boxstyle="round,pad=0.02,rounding_size=0.08",fc=c,ec=GR)); b.text(.4,y,n,fontsize=9.5,fontweight="bold",va="center"); b.text(3.9,y,d,fontsize=8.8,va="center")
plt.tight_layout(); save("hb_04_inspection_lighting.png")

# D5 roadmap
fig,ax=blank(13.5,5.6,13.5,5.6)
ax.text(6.75,5.35,"A 6-stage learning & project path (each stage ends with a portfolio artefact)",ha="center",fontsize=12,fontweight="bold")
st=[("1  Image basics","load/convert, histograms,\nfilters, thresholds,\nmorphology","Doc scanner /\nbinarisation tool"),("2  Measure things","blobs, contours, moments,\ncalibration to mm","Part-gauging\nscript with error bars"),("3  Find things","edges, Hough, corners,\nSIFT/ORB, RANSAC","Panorama stitcher /\nmarker AR demo"),("4  Learn things","features + SVM,\nCV, ROC, calibration","Classifier with proper\nevaluation report"),("5  Deep + deploy","fine-tune CNN, detector,\nexport ONNX, edge","Real-time detector\nwith FPS + accuracy"),("6  Systems","tracking, 3-D, data loop,\nmonitoring, failure modes","End-to-end product:\nvideo → decision → dashboard")]
for i,(t,k,p) in enumerate(st):
    x=1.2+i*2.2; box(ax,x,3.55,2.0,1.1,t+"\n"+k,fc="#ebf4ff",fs=7.8); box(ax,x,1.9,2.0,.9,p,fc="#c6f6d5",ec=G,fs=7.8)
    arr(ax,(x,2.98),(x,2.37),c=G)
    if i<5: arr(ax,(x+1.02,3.55),(x+1.18,3.55))
ax.text(.2,.8,"Habit that separates juniors from seniors: every project gets (a) a baseline, (b) a held-out test set that looks like production, (c) an error analysis with pictures of failures, (d) a one-page write-up.",fontsize=9,style="italic")
save("hb_05_learning_path.png"); print("ok")

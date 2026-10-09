"""Pattern-recognition lab on the sklearn digits data (offline, 1797 images of 8x8).
Shows: representation matters, curse of dimensionality, learning curves, eigen-images, confusion matrix."""
from _common import *
from sklearn.datasets import load_digits
from sklearn.model_selection import StratifiedKFold, cross_val_score, learning_curve
from sklearn.pipeline import make_pipeline
from sklearn.preprocessing import StandardScaler
from sklearn.neighbors import KNeighborsClassifier
from sklearn.discriminant_analysis import LinearDiscriminantAnalysis
from sklearn.decomposition import PCA
from sklearn.svm import SVC
from sklearn.linear_model import LogisticRegression
from sklearn.neural_network import MLPClassifier
from sklearn.metrics import confusion_matrix
from sklearn.model_selection import cross_val_predict
from skimage.feature import hog
X,y=load_digits(return_X_y=True); imgs=X.reshape(-1,8,8)
from skimage.transform import resize
H=np.array([hog(i,orientations=8,pixels_per_cell=(4,4),cells_per_block=(2,2)) for i in imgs])
H2=np.array([hog(resize(i,(32,32),order=1),orientations=9,pixels_per_cell=(8,8),cells_per_block=(2,2)) for i in imgs])
cv=StratifiedKFold(5,shuffle=True,random_state=0)
models={"raw + kNN (k=3)":(make_pipeline(StandardScaler(),KNeighborsClassifier(3)),X),
        "raw + LDA":(LinearDiscriminantAnalysis(),X),
        "raw + logistic reg.":(make_pipeline(StandardScaler(),LogisticRegression(max_iter=2000)),X),
        "raw + RBF-SVM":(make_pipeline(StandardScaler(),SVC(C=10,gamma="scale")),X),
        "PCA(20) + RBF-SVM":(make_pipeline(StandardScaler(),PCA(20),SVC(C=10,gamma="scale")),X),
        "HOG (coarse 4x4 cells) + SVM":(make_pipeline(StandardScaler(),SVC(C=10,gamma="scale")),H),
        "HOG (upscaled 32x32) + SVM":(make_pipeline(StandardScaler(),SVC(C=10,gamma="scale")),H2),
        "raw + MLP(100)":(make_pipeline(StandardScaler(),MLPClassifier((100,),max_iter=600,random_state=0)),X)}
print("5-fold CV accuracy (mean ± std)")
res={}
for k,(m,F) in models.items():
    s=cross_val_score(m,F,y,cv=cv); res[k]=s.mean(); print(f"  {k:22s} {s.mean():.3f} ± {s.std():.3f}")
# learning curve
sz,tr,te=learning_curve(make_pipeline(StandardScaler(),SVC(C=10,gamma="scale")),X,y,cv=cv,train_sizes=np.linspace(.05,1,8),random_state=0)
# curse: append irrelevant noise features
rng=np.random.RandomState(0); Xs=StandardScaler().fit_transform(X); ms=[0,20,50,100,200,400]; acc_knn=[];acc_svm=[];acc_lr=[]
for m in ms:
    Xn=np.hstack([Xs,rng.randn(len(Xs),m)]) if m else Xs
    acc_knn.append(cross_val_score(KNeighborsClassifier(3),Xn,y,cv=cv).mean()); acc_svm.append(cross_val_score(SVC(C=10,gamma="scale"),Xn,y,cv=cv).mean()); acc_lr.append(cross_val_score(LogisticRegression(max_iter=1000),Xn,y,cv=cv).mean())
print("noise features added:",ms); print(" kNN",np.round(acc_knn,3)); print(" SVM",np.round(acc_svm,3)); print(" LogReg",np.round(acc_lr,3))
# small-data regime: train on very few samples
for n in (20,50,100,300):
    r=[]
    for name,mk in [("kNN",lambda:make_pipeline(StandardScaler(),KNeighborsClassifier(1))),("SVM",lambda:make_pipeline(StandardScaler(),SVC(C=10,gamma="scale"))),("LDA",lambda:LinearDiscriminantAnalysis(solver="svd"))]:
        sc=[];
        for seed in range(5):
            idx=np.random.RandomState(seed).permutation(len(X)); a,b=idx[:n],idx[n:]; sc.append(mk().fit(X[a],y[a]).score(X[b],y[b]))
        r.append(f"{name} {np.mean(sc):.3f}")
    print(f"  train size {n:4d}:",", ".join(r))
# figures
fig,axs=plt.subplots(1,3,figsize=(14,3.8))
a=axs[0]; names=list(res); a.barh(names[::-1],[res[n] for n in names[::-1]],color=[G if "upscaled" in n else (O if "HOG" in n else B) for n in names[::-1]]); a.set_xlim(.88,1.0); a.set_title("5-fold CV accuracy: representation + model",fontsize=10)
for i,n in enumerate(names[::-1]): a.text(res[n]+.001,i,f"{res[n]:.3f}",va="center",fontsize=8)
a=axs[1]; a.plot(sz,tr.mean(1),"o-",color=B,label="train"); a.plot(sz,te.mean(1),"o-",color=R,label="validation"); a.set_xlabel("training examples"); a.set_ylabel("accuracy"); a.set_title("Learning curve (RBF-SVM): more data closes the gap",fontsize=10); a.legend(); a.grid(alpha=.3)
a=axs[2]; a.plot(ms,acc_knn,"o-",color=R,label="kNN"); a.plot(ms,acc_svm,"o-",color=B,label="RBF-SVM"); a.plot(ms,acc_lr,"o-",color=G,label="logistic"); a.set_xlabel("# irrelevant noise features added to 64 pixels"); a.set_title("Curse of dimensionality: distance-based hurt most",fontsize=10); a.legend(); a.grid(alpha=.3)
plt.tight_layout(); save("pr_01_digits_results.png")
pca=PCA(16).fit(Xs); fig,axs=plt.subplots(2,9,figsize=(13,3.4)); axs[0,0].imshow(X.mean(0).reshape(8,8),cmap="gray"); axs[0,0].set_title("mean",fontsize=8)
for k in range(8): axs[0,k+1].imshow(pca.components_[k].reshape(8,8),cmap="RdBu_r"); axs[0,k+1].set_title(f"PC{k+1} ({100*pca.explained_variance_ratio_[k]:.0f}%)",fontsize=8)
rec=lambda k: (pca.mean_+ (pca.transform(Xs[:1])[:, :k]@pca.components_[:k]))
for j,k in enumerate([1,2,5,10,16]):
    Z=PCA(k).fit(Xs); r=Z.inverse_transform(Z.transform(Xs[:1])); axs[1,j].imshow(r.reshape(8,8),cmap="gray"); axs[1,j].set_title(f"{k} comps",fontsize=8)
axs[1,5].imshow(Xs[0].reshape(8,8),cmap="gray"); axs[1,5].set_title("original",fontsize=8)
for a in axs.ravel(): a.axis("off")
plt.suptitle("PCA = 'eigen-digits': the same idea as eigenfaces (Baggio ch.8, DHS ch.10)",fontsize=10); plt.tight_layout(); save("pr_02_eigen_digits.png")
best=make_pipeline(StandardScaler(),SVC(C=10,gamma="scale")); pred=cross_val_predict(best,H2,y,cv=cv); cm=confusion_matrix(y,pred)
fig,ax=plt.subplots(figsize=(5.4,4.8)); ax.imshow(np.log1p(cm),cmap="Blues"); 
for (i,j),v in np.ndenumerate(cm): 
    if v: ax.text(j,i,v,ha="center",va="center",fontsize=7,color="white" if v>60 else "black")
ax.set_xlabel("predicted"); ax.set_ylabel("true"); ax.set_xticks(range(10)); ax.set_yticks(range(10)); ax.set_title(f"Confusion matrix, upscaled-HOG+SVM (acc {np.mean(pred==y):.3f})",fontsize=10); save("pr_03_confusion.png")
off=[(i,j,cm[i,j]) for i in range(10) for j in range(10) if i!=j and cm[i,j]>=3]; print("largest confusions (true,pred,count):",sorted(off,key=lambda t:-t[2])[:6])

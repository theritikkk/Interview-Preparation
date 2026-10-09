"""Decision theory in 60 lines: priors, costs, ROC, reject option (Duda ch.2; Davies 24.3-24.7)."""
from _common import *
from scipy.stats import norm
from sklearn.metrics import roc_auc_score, roc_curve
rng=np.random.RandomState(0); N=200000; p1=0.05                     # 5% defective parts (rare class)
y=(rng.rand(N)<p1).astype(int); x=np.where(y==1,rng.normal(2.0,1,N),rng.normal(0,1,N))
post=lambda x: p1*norm.pdf(x,2,1)/(p1*norm.pdf(x,2,1)+(1-p1)*norm.pdf(x,0,1))
PP=post(x); print("AUC",round(roc_auc_score(y,x),4))
def stats(pred): 
    tp=((pred==1)&(y==1)).sum(); fp=((pred==1)&(y==0)).sum(); fn=((pred==0)&(y==1)).sum(); return tp/(tp+fn),fp/(y==0).sum(),tp/max(tp+fp,1),(pred==y).mean()
for name,thr in [("Bayes min-error (posterior>0.5)",0.5),("cost-sensitive C_FN=20,C_FP=1 (posterior>1/21)",1/21),("naive 'always good' ",2)]:
    pred=(PP>thr).astype(int); tpr,fpr,prec,acc=stats(pred); cost=((pred==0)&(y==1)).mean()*20+((pred==1)&(y==0)).mean()*1
    print(f"  {name:48s} recall={tpr:.3f} FPR={fpr:.3f} precision={prec:.3f} accuracy={acc:.3f} expected cost/part={cost:.4f}")
# threshold on x for those posteriors
from scipy.optimize import brentq
for thr in (0.5,1/21): print("   decision threshold on feature x for posterior",round(thr,3),"=",round(brentq(lambda t:post(t)-thr,-3,6),3))
# reject option on max posterior
mp=np.maximum(PP,1-PP); order=np.argsort(mp); out=[]
for rr in [0,.02,.05,.1,.2,.3]:
    keep=np.ones(N,bool); keep[order[:int(rr*N)]]=False; out.append((rr,((PP[keep]>.5)==y[keep]).mean()))
print("reject rate -> accuracy on accepted:",[(a,round(b,4)) for a,b in out])
fpr,tpr,th=roc_curve(y,x)
fig,axs=plt.subplots(1,3,figsize=(14,3.8))
a=axs[0]; xs=np.linspace(-4,6,400); a.fill_between(xs,(1-p1)*norm.pdf(xs,0,1),color=B,alpha=.35,label="good parts (95%)"); a.fill_between(xs,p1*norm.pdf(xs,2,1),color=R,alpha=.5,label="defective (5%)")
for t,c,l in [(brentq(lambda t:post(t)-.5,-3,6),"k","min-error"),(brentq(lambda t:post(t)-1/21,-3,6),G,"cost-aware")]: a.axvline(t,color=c,ls="--"); a.text(t+.05,.33,l,rotation=90,fontsize=8,color=c)
a.set_title("Priors × likelihoods: with a rare class the\nmin-error threshold sits far right",fontsize=9); a.legend(fontsize=7); a.set_xlabel("feature x")
a=axs[1]; a.plot(fpr,tpr,color=B,lw=2); a.plot([0,1],[0,1],":",color=GR)
for t,c in [(brentq(lambda t:post(t)-.5,-3,6),"k"),(brentq(lambda t:post(t)-1/21,-3,6),G)]:
    i=np.argmin(abs(th-t)); a.scatter(fpr[i],tpr[i],c=c,s=60,zorder=5)
a.set_xlabel("false-positive rate"); a.set_ylabel("true-positive rate (recall)"); a.set_title(f"ROC (AUC={roc_auc_score(y,x):.3f}); dots = operating points",fontsize=9)
a=axs[2]; a.plot([r*100 for r,_ in out],[b*100 for _,b in out],"o-",color=P); a.set_xlabel("% of samples rejected (least confident)"); a.set_ylabel("accuracy on the accepted samples (%)"); a.set_title("Error-reject trade-off (Davies 24.6)",fontsize=9); a.grid(alpha=.3)
plt.tight_layout(); save("pr_04_bayes_roc_cost.png")

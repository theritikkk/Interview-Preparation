import numpy as np, matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import os
IMG=os.path.join(os.path.dirname(os.path.abspath(__file__)),"..","images")+os.sep
plt.rcParams.update({"font.size":10,"axes.spines.top":False,"axes.spines.right":False,"figure.dpi":110,"mathtext.fontset":"cm"})
B,R,G,O,P,GR="#2b6cb0","#c53030","#2f855a","#dd6b20","#6b46c1","#718096"
def save(n): plt.savefig(IMG+n,dpi=120,bbox_inches="tight",facecolor="white"); plt.close()

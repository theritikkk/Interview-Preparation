"""End-to-end demo with ground truth: calibrate (good vs poor protocol) -> validate -> measure parts -> compare with the naive baseline.
Run:  python run_demo.py      (about 1-2 minutes; writes results/summary.json, results/measurements.csv, results/figure.png)"""
import json, csv, numpy as np, cv2, matplotlib
matplotlib.use("Agg"); import matplotlib.pyplot as plt
from gauge import *
rng = np.random.RandomState(0)
def cal_set(n, tilt, seed0):
    r = np.random.RandomState(seed0); imgs = []
    for i in range(n):
        rv, tv = random_board_pose(r, tilt_deg=tilt); imgs.append(render(rv, tv, scene="board", blur=1.0, noise=2.0, seed=seed0 * 100 + i))
    return imgs
print("1) calibration images: 18 good (tilts up to 35 deg), 8 poor (near-parallel, tilts <= 4 deg), 5 held-out validation")
good = cal_set(18, 35, 1); poor = cal_set(8, 4, 2); held = cal_set(5, 35, 3)
def report(name, cal):
    K, d = cal["K"], cal["dist"]
    print(f"   {name:5s}: fx={K[0,0]:8.2f} (true 1200)  fy={K[1,1]:8.2f}  cx={K[0,2]:7.2f} (512)  cy={K[1,2]:7.2f} (384)  k1={d[0]:+.4f} (-0.20)  k2={d[1]:+.4f} (+0.06)  |  training RMS {cal['rms']:.3f} px, held-out RMS {np.mean(validate(K,d,held)):.3f} px")
cal_good = calibrate(good); cal_poor = calibrate(poor); save(cal_good, "results/calibration.json")
report("good", cal_good); report("poor", cal_poor)
print("   -> the poor calibration also has a tiny reprojection error, yet its focal length is wrong: reprojection error is not proof.")
print("2) measurement: 20 captures with small camera re-positioning (jitter) and fresh noise")
methods = {"naive": None, "poor calibration": cal_poor, "good calibration": cal_good, "oracle (true K)": dict(K=TRUE_K, dist=TRUE_DIST)}
truth = {p["name"]: p for p in PARTS}; rows = []; example = None
for cap in range(20):
    rv, tv = measurement_pose(rng, jitter=True); img = render(rv, tv, scene="measure", blur=1.0, noise=2.0, seed=500 + cap)
    ctr_px = {}
    for p in PARTS:
        pts, _ = cv2.projectPoints(np.float64([[p["c"][0], p["c"][1], 0]]), rv, tv, TRUE_K, TRUE_DIST); ctr_px[p["name"]] = pts.reshape(2)
    for m, cal in methods.items():
        res = measure_parts(img, None if cal is None else cal["K"], None if cal is None else cal["dist"], naive=(m == "naive"))
        if res is None: continue
        for part in res[0]:
            name = min(ctr_px, key=lambda k: np.hypot(*(np.array(part["centre_px"]) - ctr_px[k])))
            if np.hypot(*(np.array(part["centre_px"]) - ctr_px[name])) > 15: continue
            t = truth[name]
            if t["kind"] == "circle": dims = [("diameter", part.get("diameter_mm", np.nan), t["d"])]
            else: dims = [("length", part.get("length_mm", np.nan), max(t["size"])), ("width", part.get("width_mm", np.nan), min(t["size"]))]
            for dn, meas, tr in dims: rows.append(dict(capture=cap, method=m, part=name, dim=dn, measured=float(meas), truth=float(tr), error=float(meas - tr)))
        if m == "good calibration" and cap == 0: example = (img, res[0])
import collections
summ = {}; print(f"\n   {'method':18s} | mean |error| mm | worst |error| mm | bias mm (mean signed) | repeatability std mm (mean over dims)")
for m in methods:
    R = [r for r in rows if r["method"] == m]
    if not R: continue
    e = np.array([r["error"] for r in R]); per = collections.defaultdict(list)
    for r in R: per[(r["part"], r["dim"])].append(r["error"])
    rep = np.mean([np.std(v) for v in per.values() if len(v) > 3]); summ[m] = dict(mean_abs_error_mm=float(np.mean(np.abs(e))), worst_abs_error_mm=float(np.max(np.abs(e))), bias_mm=float(e.mean()), repeatability_std_mm=float(rep), n=len(R))
    print(f"   {m:18s} | {np.mean(np.abs(e)):15.3f} | {np.max(np.abs(e)):16.3f} | {e.mean():21.3f} | {rep:.3f}   (n={len(R)})")
print("\n   per part (good calibration): mean error ± repeatability std over 20 captures [mm]")
for (pn, dn), v in sorted(collections.defaultdict(list, {(r["part"], r["dim"]): [] for r in rows}).items()): pass
pp = collections.defaultdict(list)
for r in rows:
    if r["method"] == "good calibration": pp[(r["part"], r["dim"], r["truth"])].append(r["error"])
for (pn, dn, tr), v in sorted(pp.items()): print(f"     {pn:18s} {dn:8s} truth {tr:5.1f}: error {np.mean(v):+.3f} ± {np.std(v):.3f}  (n={len(v)})")
json.dump(dict(calibration_good=dict(fx=cal_good["K"][0, 0], fy=cal_good["K"][1, 1], cx=cal_good["K"][0, 2], cy=cal_good["K"][1, 2], dist=list(cal_good["dist"]), rms=cal_good["rms"], heldout_rms=float(np.mean(validate(cal_good["K"], cal_good["dist"], held)))),
          calibration_poor=dict(fx=cal_poor["K"][0, 0], rms=cal_poor["rms"], heldout_rms=float(np.mean(validate(cal_poor["K"], cal_poor["dist"], held)))), methods=summ), open("results/summary.json", "w"), indent=2, default=float)
with open("results/measurements.csv", "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=list(rows[0].keys())); w.writeheader(); w.writerows(rows)
# ---- figure
fig, axs = plt.subplots(1, 3, figsize=(16, 4.4)); img, parts = example; vis = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)
for p in parts:
    cx, cy = map(int, p["centre_px"]); txt = f"d={p['diameter_mm']:.2f}" if p["shape"] == "circle" else f"{p['length_mm']:.2f}x{p['width_mm']:.2f}"
    cv2.putText(vis, txt, (cx - 45, cy + 4), cv2.FONT_HERSHEY_SIMPLEX, 0.45, (0, 255, 255), 1, cv2.LINE_AA)
axs[0].imshow(vis[..., ::-1]); axs[0].set_title("capture (barrel distortion + perspective) with measurements in mm", fontsize=9); axs[0].axis("off")
names = [m for m in methods if m in summ]; x = np.arange(len(names))
axs[1].bar(x - 0.2, [summ[m]["mean_abs_error_mm"] for m in names], 0.4, label="mean |error|", color="#2b6cb0"); axs[1].bar(x + 0.2, [summ[m]["repeatability_std_mm"] for m in names], 0.4, label="repeatability (std)", color="#dd6b20")
axs[1].set_xticks(x); axs[1].set_xticklabels([n.replace(" ", "\n") for n in names], fontsize=8); axs[1].set_ylabel("mm"); axs[1].legend(fontsize=8); axs[1].set_title("accuracy and repeatability by method (20 captures, 5 parts)", fontsize=9)
for m, c in [("naive", "#c53030"), ("good calibration", "#2f855a")]:
    e = [r["error"] for r in rows if r["method"] == m]; axs[2].hist(e, bins=30, alpha=0.6, color=c, label=m)
axs[2].set_xlabel("measurement error (mm)"); axs[2].legend(fontsize=8); axs[2].set_title("error distribution: naive vs calibrated", fontsize=9)
plt.tight_layout(); plt.savefig("results/figure.png", dpi=120, bbox_inches="tight", facecolor="white"); print("\nwrote results/summary.json, results/measurements.csv, results/figure.png")

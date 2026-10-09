"""Command line: calibrate from your checkerboard photos, then measure parts in a photo taken with the SAME camera setup.
  python -m gauge.cli calibrate "data/cal/*.png" --out results/calibration.json
  python -m gauge.cli measure data/part.png --calibration results/calibration.json [--annotate results/out.png]"""
import argparse, glob, json, sys, cv2, numpy as np
from .calibrate import calibrate as _calibrate, validate as _validate, save as _save, load as _load   # (importing the names avoids the module/function clash)
from .measure import measure_parts

def _read(path):
    g = cv2.imread(path, cv2.IMREAD_GRAYSCALE)
    if g is None: raise SystemExit(f"cannot read image: {path}")
    return g

def main(argv=None):
    ap = argparse.ArgumentParser(prog="gauge"); sub = ap.add_subparsers(dest="cmd", required=True)
    a = sub.add_parser("calibrate"); a.add_argument("pattern"); a.add_argument("--out", default="results/calibration.json"); a.add_argument("--holdout", type=int, default=3, help="number of images kept aside for independent validation")
    m = sub.add_parser("measure"); m.add_argument("image"); m.add_argument("--calibration", required=True); m.add_argument("--annotate")
    args = ap.parse_args(argv)
    if args.cmd == "calibrate":
        files = sorted(glob.glob(args.pattern))
        if len(files) < 8: raise SystemExit(f"found {len(files)} images; need at least 8 (ideally 15-25 with large tilts)")
        imgs = [_read(f) for f in files]; hold = imgs[-args.holdout:] if args.holdout else []; train = imgs[:-args.holdout] if args.holdout else imgs
        cal = _calibrate(train); _save(cal, args.out)
        print(f"used {cal['n_used']}/{len(train)} images | fx={cal['K'][0,0]:.1f} fy={cal['K'][1,1]:.1f} cx={cal['K'][0,2]:.1f} cy={cal['K'][1,2]:.1f} | dist={np.round(cal['dist'],4).tolist()}")
        print(f"training reprojection RMS: {cal['rms']:.3f} px")
        if hold:
            v = _validate(cal["K"], cal["dist"], hold)
            if v: print(f"HELD-OUT reprojection RMS: {np.mean(v):.3f} px  ({len(v)} images)  <- trust this more than the training RMS")
            if v and np.mean(v) > 2.5 * cal["rms"]: print("WARNING: held-out error is much larger than training error -> poor pose diversity or too few views; recapture.")
        print("saved", args.out)
    else:
        cal = _load(args.calibration); img = _read(args.image); res = measure_parts(img, cal["K"], cal["dist"])
        if res is None: raise SystemExit("checkerboard not found in the measurement image (it must be visible, in the same plane as the parts)")
        parts, rms = res; print(f"board pose reprojection RMS: {rms:.3f} px   (parts found: {len(parts)})"); vis = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)
        for i, p in enumerate(parts, 1):
            d = f"circle  diameter {p['diameter_mm']:.2f} mm" if p["shape"] == "circle" else f"rect    {p['length_mm']:.2f} x {p['width_mm']:.2f} mm"
            print(f"  #{i}  {d}   area {p['area_mm2']:.1f} mm^2   centre ({p['centre_mm'][0]:.1f}, {p['centre_mm'][1]:.1f}) mm")
            cx, cy = map(int, p["centre_px"]); cv2.putText(vis, str(i), (cx - 6, cy + 5), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2, cv2.LINE_AA)
        if args.annotate: cv2.imwrite(args.annotate, vis); print("annotated image:", args.annotate)

if __name__ == "__main__": main()

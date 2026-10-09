"""Camera calibration from checkerboard images, with an INDEPENDENT validation step (reprojection error alone is not proof)."""
import json, numpy as np, cv2
from .synth import PATTERN, board_object_points

def find_corners(gray):
    """Sub-pixel inner-corner detection (sector-based detector; robust to blur/lighting). Returns (N,2) float64 or None."""
    ok, c = cv2.findChessboardCornersSB(gray, PATTERN, cv2.CALIB_CB_EXHAUSTIVE | cv2.CALIB_CB_ACCURACY)
    return c.reshape(-1, 2).astype(np.float64) if ok else None

def reproj_rms(objp, imgp, rvec, tvec, K, dist):
    p, _ = cv2.projectPoints(objp, rvec, tvec, K, dist); return float(np.sqrt(np.mean(np.sum((p.reshape(-1, 2) - imgp) ** 2, axis=1))))

def calibrate(gray_images, flags=0):
    """Return dict(K, dist, rms, per_view_rms, n_used). Needs >= 8 views with large tilts; see README for the protocol."""
    objp = board_object_points(); O, I, used = [], [], 0
    for g in gray_images:
        c = find_corners(g)
        if c is not None: O.append(objp.astype(np.float32)); I.append(c.astype(np.float32)); used += 1
    if used < 5: raise ValueError(f"only {used} usable views; need at least 5 (ideally 15-25)")
    h, w = gray_images[0].shape; K0 = np.array([[1.0 * w, 0, w / 2], [0, 1.0 * w, h / 2], [0, 0, 1]])
    rms, K, dist, rv, tv = cv2.calibrateCamera(O, I, (w, h), K0, None, flags=flags | cv2.CALIB_USE_INTRINSIC_GUESS)
    per = [reproj_rms(objp, I[i].astype(np.float64), rv[i], tv[i], K, dist) for i in range(used)]
    return dict(K=K, dist=dist.ravel(), rms=float(rms), per_view_rms=per, n_used=used, image_size=(w, h))

def validate(K, dist, gray_images):
    """Independent check: pose each HELD-OUT board image with the calibration and report its reprojection RMS (px)."""
    objp = board_object_points(); out = []
    for g in gray_images:
        c = find_corners(g)
        if c is None: continue
        ok, rv, tv = cv2.solvePnP(objp, c, K, dist, flags=cv2.SOLVEPNP_ITERATIVE)
        if ok: out.append(reproj_rms(objp, c, rv, tv, K, dist))
    return out

def save(cal, path):
    json.dump(dict(K=np.asarray(cal["K"]).tolist(), dist=np.asarray(cal["dist"]).tolist(), rms=cal["rms"], n_used=cal["n_used"], image_size=list(cal["image_size"])), open(path, "w"), indent=2)

def load(path):
    d = json.load(open(path)); return dict(K=np.array(d["K"]), dist=np.array(d["dist"]), rms=d["rms"], n_used=d["n_used"], image_size=tuple(d["image_size"]))

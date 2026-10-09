"""Measure parts lying on the calibration plane, in millimetres:
 board pose (PnP) -> segment parts -> sub-pixel contour -> rays -> intersection with the plane -> fit/measure."""
import numpy as np, cv2
from skimage import measure as skm, filters as skf
from .synth import PATTERN, SQ, board_object_points
from .calibrate import find_corners

def plane_pose(gray, K, dist):
    """Pose of the board (= measurement plane Z=0) in camera coordinates, or None. Returns (rvec, tvec, reproj_rms_px)."""
    c = find_corners(gray)
    if c is None: return None
    objp = board_object_points(); ok, rv, tv = cv2.solvePnP(objp, c, K, dist, flags=cv2.SOLVEPNP_ITERATIVE)
    if not ok: return None
    p, _ = cv2.projectPoints(objp, rv, tv, K, dist); return rv, tv, float(np.sqrt(np.mean(np.sum((p.reshape(-1, 2) - c) ** 2, 1))))

def pixels_to_plane(uv, K, dist, rvec, tvec):
    """Undistort pixels, cast rays, intersect with the board plane (Z=0). Returns (N,2) millimetres in board coordinates."""
    uv = np.asarray(uv, np.float64).reshape(-1, 1, 2); n = cv2.undistortPoints(uv, K, dist).reshape(-1, 2)
    R, _ = cv2.Rodrigues(np.asarray(rvec, np.float64)); t = np.asarray(tvec, np.float64).reshape(3)
    d = np.c_[n, np.ones(len(n))]                       # ray directions in the camera frame
    Rt_d = d @ R                                        # (R^T d_c) for every ray
    s = (R.T @ t)[2] / Rt_d[:, 2]                       # solve Z_world = 0
    Xw = (s[:, None] * d - t) @ R                       # R^T (s d - t)
    return Xw[:, :2]

def fit_circle(xy):
    """Algebraic (Kasa) fit followed by a few Gauss-Newton steps on the geometric error. Returns (cx, cy, r, rms_residual)."""
    x, y = xy[:, 0], xy[:, 1]; A = np.c_[2 * x, 2 * y, np.ones(len(x))]; b = x ** 2 + y ** 2
    cx, cy, c = np.linalg.lstsq(A, b, rcond=None)[0]; r = np.sqrt(c + cx ** 2 + cy ** 2)
    for _ in range(10):
        dx, dy = x - cx, y - cy; rho = np.hypot(dx, dy) + 1e-12; res = rho - r
        J = np.c_[-dx / rho, -dy / rho, -np.ones(len(x))]; step = np.linalg.lstsq(J, -res, rcond=None)[0]; cx, cy, r = cx + step[0], cy + step[1], r + step[2]
    return float(cx), float(cy), float(r), float(np.sqrt(np.mean((np.hypot(x - cx, y - cy) - r) ** 2)))

def polygon_area(xy):
    x, y = xy[:, 0], xy[:, 1]; return 0.5 * abs(np.dot(x, np.roll(y, -1)) - np.dot(y, np.roll(x, -1)))

def _board_mask(shape, K, dist, rvec, tvec, margin_mm=12):
    bx, by = (PATTERN[0] + 1) * SQ + margin_mm, (PATTERN[1] + 1) * SQ + margin_mm
    corners = np.float64([[-margin_mm, -margin_mm, 0], [bx, -margin_mm, 0], [bx, by, 0], [-margin_mm, by, 0]])
    p, _ = cv2.projectPoints(corners, rvec, tvec, K, dist); m = np.zeros(shape, np.uint8); cv2.fillPoly(m, [p.reshape(-1, 2).astype(np.int32)], 255); return m > 0

def _naive_scale_and_mask(gray):
    """BASELINE (what people do without calibration): one mm-per-pixel scale from the board squares, no undistortion, no perspective."""
    c = find_corners(gray)
    if c is None: return None
    P = c.reshape(PATTERN[1], PATTERN[0], 2); sp = np.mean([np.linalg.norm(P[:, 1:] - P[:, :-1], axis=2).mean(), np.linalg.norm(P[1:] - P[:-1], axis=2).mean()])
    hull = cv2.convexHull(c.astype(np.float32)).reshape(-1, 2); ctr = hull.mean(0); hull = ctr + (hull - ctr) * 1.45
    m = np.zeros(gray.shape, np.uint8); cv2.fillPoly(m, [hull.astype(np.int32)], 255); return SQ / sp, m > 0

def measure_parts(gray, K, dist, min_area_px=400, naive=False):
    """Return (parts, board_reproj_rms) or None if the board is not found. Each part: shape, dimensions (mm), centre, area, centre_px.
    naive=True gives the uncalibrated single-scale baseline for comparison."""
    if naive:
        nm = _naive_scale_and_mask(gray)
        if nm is None: return None
        naive_s, bmask = nm; rvec = tvec = None; rms = float("nan")
    else:
        pp = plane_pose(gray, K, dist)
        if pp is None: return None
        rvec, tvec, rms = pp; bmask = _board_mask(gray.shape, K, dist, rvec, tvec)
    g = cv2.GaussianBlur(gray.astype(np.float32), (0, 0), 1.0)
    t0 = skf.threshold_multiotsu(g.astype(np.uint8), classes=3)[0]                                    # table / paper / dark parts -> darkest class
    bw = ((g < t0) * 255).astype(np.uint8)
    bw[bmask] = 0; bw[:6, :] = 0; bw[-6:, :] = 0; bw[:, :6] = 0; bw[:, -6:] = 0
    bw = cv2.morphologyEx(bw, cv2.MORPH_OPEN, np.ones((3, 3), np.uint8))
    n, lab, st, _ = cv2.connectedComponentsWithStats(bw, connectivity=8); out = []
    for i in range(1, n):
        x0, y0, w, h, area = st[i]
        if area < min_area_px or area > 0.2 * gray.size: continue
        pad = 12; xa, ya, xb, yb = max(x0 - pad, 0), max(y0 - pad, 0), min(x0 + w + pad, gray.shape[1]), min(y0 + h + pad, gray.shape[0])
        roi = g[ya:yb, xa:xb]; comp = (lab[ya:yb, xa:xb] == i)
        ring = cv2.dilate(comp.astype(np.uint8), np.ones((15, 15), np.uint8)) & (~cv2.dilate(comp.astype(np.uint8), np.ones((7, 7), np.uint8)).astype(bool)).astype(np.uint8)
        inside, outside = np.median(roi[comp]), np.median(roi[ring.astype(bool)]); level = 0.5 * (inside + outside)       # iso-level = half-way contrast
        cs = skm.find_contours(roi, level); cs = [c for c in cs if len(c) > 20 and np.allclose(c[0], c[-1], atol=1.5)]
        if not cs: continue
        c = max(cs, key=len); uv = np.c_[c[:, 1] + xa, c[:, 0] + ya]                                                          # (u, v) sub-pixel
        mm = uv * naive_s if naive else pixels_to_plane(uv, K, dist, rvec, tvec); A = polygon_area(mm); cpx = (float(uv[:, 0].mean()), float(uv[:, 1].mean()))
        P = np.sum(np.hypot(*np.diff(np.vstack([mm, mm[:1]]), axis=0).T)); circ = 4 * np.pi * A / P ** 2
        if circ > 0.93:
            cx, cy, r, res = fit_circle(mm); out.append(dict(shape="circle", diameter_mm=2 * r, centre_mm=(cx, cy), area_mm2=A, contour_rms_mm=res, px_area=int(area), centre_px=cpx))
        else:
            (cx, cy), (rw, rh), ang = cv2.minAreaRect(mm.astype(np.float32)); a, b = sorted([rw, rh], reverse=True)
            out.append(dict(shape="rect", length_mm=a, width_mm=b, centre_mm=(float(cx), float(cy)), area_mm2=A, px_area=int(area), centre_px=cpx))
    return sorted(out, key=lambda d: d["centre_mm"][0] * 10 + d["centre_mm"][1]), rms

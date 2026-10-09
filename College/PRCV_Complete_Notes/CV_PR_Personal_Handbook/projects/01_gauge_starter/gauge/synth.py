"""Synthetic scene generator with KNOWN ground truth (lens distortion, perspective, blur, noise).
Lets you test the whole pipeline without hardware; swap in real photos later (see README)."""
import numpy as np, cv2
SQ = 25.0                      # checkerboard square size, mm
PATTERN = (9, 6)               # inner corners (columns, rows)  -> 10 x 7 squares = 250 x 175 mm
SHEET = (-20.0, -20.0, 520.0, 200.0)   # white paper sheet on a grey table (x0, y0, x1, y1) in mm
IMG_SIZE = (1024, 768)         # (width, height) px
TRUE_K = np.array([[1200., 0, 512.], [0, 1200., 384.], [0, 0, 1.]])
TRUE_DIST = np.array([-0.20, 0.06, 0.0, 0.0, 0.0])      # k1, k2, p1, p2, k3  (visible barrel distortion)
PARTS = [  # ground-truth parts lying on the plane next to the board (millimetres)
    dict(name="rect_60x40",        kind="rect",   c=(330, 45),  size=(60, 40), ang=0.0),
    dict(name="circle_30",         kind="circle", c=(330, 130), d=30.0),
    dict(name="rect_40x25_rot20",  kind="rect",   c=(430, 50),  size=(40, 25), ang=20.0),
    dict(name="circle_20",         kind="circle", c=(430, 130), d=20.0),
    dict(name="bar_90x12",         kind="rect",   c=(400, 175), size=(90, 12), ang=0.0),
]

def board_object_points():
    """3-D coordinates (mm, plane Z=0) of the inner corners in the order OpenCV returns them (row-major)."""
    objp = np.zeros((PATTERN[0] * PATTERN[1], 3), np.float64); k = 0
    for j in range(PATTERN[1]):
        for i in range(PATTERN[0]):
            objp[k] = ((i + 1) * SQ, (j + 1) * SQ, 0.0); k += 1
    return objp

def _texture(X, Y, scene):
    img = np.full(X.shape, 150.0)                                   # table
    x0, y0, x1, y1 = SHEET; img[(X >= x0) & (X <= x1) & (Y >= y0) & (Y <= y1)] = 225.0   # paper
    bx, by = (PATTERN[0] + 1) * SQ, (PATTERN[1] + 1) * SQ
    inb = (X >= 0) & (X <= bx) & (Y >= 0) & (Y <= by)
    chk = ((np.floor(X / SQ) + np.floor(Y / SQ)) % 2 == 0)
    img[inb] = np.where(chk[inb], 235.0, 30.0)                      # checkerboard
    if scene == "measure":
        for p in PARTS:
            dx, dy = X - p["c"][0], Y - p["c"][1]
            if p["kind"] == "circle": m = np.hypot(dx, dy) <= p["d"] / 2
            else:
                a = np.radians(p["ang"]); u = dx * np.cos(a) + dy * np.sin(a); v = -dx * np.sin(a) + dy * np.cos(a)
                m = (np.abs(u) <= p["size"][0] / 2) & (np.abs(v) <= p["size"][1] / 2)
            img[m] = 40.0
    return img

def pose_from_target(target_xy, dist_mm, rvec):
    """Camera looks along its +z axis at plane point `target_xy` from `dist_mm` away; `rvec` = world->camera rotation."""
    R, _ = cv2.Rodrigues(np.asarray(rvec, np.float64)); T = np.array([target_xy[0], target_xy[1], 0.0])
    C = T - dist_mm * (R.T @ np.array([0, 0, 1.0])); t = -R @ C
    return np.asarray(rvec, np.float64).reshape(3), t

def render(rvec, tvec, scene="board", blur=1.0, noise=2.0, seed=0, ss=2, K=TRUE_K, dist=TRUE_DIST, size=IMG_SIZE):
    """Ray-trace the plane Z=0 through a distorting pinhole camera, with ss x ss super-sampling. Returns uint8 gray image."""
    W, H = size; rng = np.random.RandomState(seed)
    R, _ = cv2.Rodrigues(np.asarray(rvec, np.float64)); C = -R.T @ np.asarray(tvec, np.float64).reshape(3)
    uu, vv = np.meshgrid(np.arange(W), np.arange(H)); acc = np.zeros((H, W)); offs = (np.arange(ss) + 0.5) / ss - 0.5
    for dy in offs:
        for dx in offs:
            pts = np.stack([uu + dx, vv + dy], -1).reshape(-1, 1, 2).astype(np.float64)
            n = cv2.undistortPoints(pts, K, dist).reshape(-1, 2)          # ideal normalised rays
            d = np.c_[n, np.ones(len(n))] @ R                              # world-frame ray directions (R^T d_c)
            s = -C[2] / d[:, 2]; P = C[None] + s[:, None] * d
            acc += _texture(P[:, 0].reshape(H, W), P[:, 1].reshape(H, W), scene)
    img = acc / (ss * ss)
    img *= (0.92 + 0.12 * (uu / W))                                       # mild lighting gradient
    if blur > 0: img = cv2.GaussianBlur(img, (0, 0), blur)               # lens/focus blur
    img = img + rng.randn(H, W) * noise                                   # sensor noise
    return np.clip(img, 0, 255).astype(np.uint8)

def random_board_pose(rng, tilt_deg=35.0):
    """A random, usable calibration pose: board near the image centre, tilted, at 45-80 cm."""
    for _ in range(100):
        ax = rng.randn(3); ax /= np.linalg.norm(ax); rv = ax * np.radians(rng.uniform(0, tilt_deg))
        rvec, tvec = pose_from_target((125 + rng.uniform(-40, 40), 87.5 + rng.uniform(-30, 30)), rng.uniform(450, 800), rv)
        pts, _ = cv2.projectPoints(board_object_points(), rvec, tvec, TRUE_K, TRUE_DIST); pts = pts.reshape(-1, 2)
        if (pts[:, 0] > 25).all() and (pts[:, 0] < IMG_SIZE[0] - 25).all() and (pts[:, 1] > 25).all() and (pts[:, 1] < IMG_SIZE[1] - 25).all():
            return rvec, tvec
    raise RuntimeError("could not find a visible pose")

def measurement_pose(rng=None, jitter=True):
    """Fixed-ish camera above the sheet (about 10 deg tilt); `jitter` simulates small re-positioning between captures."""
    rv = np.radians([10.0, -6.0, 3.0]); tgt = (250.0, 90.0); dist = 760.0
    if jitter and rng is not None:
        rv = rv + np.radians(rng.uniform(-2, 2, 3)); tgt = (tgt[0] + rng.uniform(-10, 10), tgt[1] + rng.uniform(-8, 8)); dist += rng.uniform(-20, 20)
    return pose_from_target(tgt, dist, rv)

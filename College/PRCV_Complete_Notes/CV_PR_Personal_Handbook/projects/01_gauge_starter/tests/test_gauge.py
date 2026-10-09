"""Plain-assert tests (run with `python tests/test_gauge.py`, or `pytest` if installed)."""
import sys, os; sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
import numpy as np, cv2
from gauge import *
from gauge.measure import pixels_to_plane, fit_circle, polygon_area

def test_object_points_spacing():
    o = board_object_points(); assert o.shape == (54, 3) and np.allclose(o[1] - o[0], [SQ, 0, 0]) and np.allclose(o[9] - o[0], [0, SQ, 0])

def test_pixels_to_plane_roundtrip():
    rv, tv = measurement_pose(jitter=False); xy = np.array([[10., 20.], [300., 40.], [450., 175.], [120., 90.]]); w3 = np.c_[xy, np.zeros(len(xy))]
    uv, _ = cv2.projectPoints(w3, rv, tv, TRUE_K, TRUE_DIST); back = pixels_to_plane(uv.reshape(-1, 2), TRUE_K, TRUE_DIST, rv, tv)
    assert np.abs(back - xy).max() < 1e-3, np.abs(back - xy).max()          # distortion + perspective are inverted exactly

def test_fit_circle_recovers_radius_and_centre():
    t = np.linspace(0, 2 * np.pi, 200, endpoint=False); r = np.random.RandomState(0); pts = np.c_[5 + 12.5 * np.cos(t), -3 + 12.5 * np.sin(t)] + r.randn(200, 2) * 0.02
    cx, cy, rad, _ = fit_circle(pts); assert abs(cx - 5) < 0.02 and abs(cy + 3) < 0.02 and abs(rad - 12.5) < 0.02

def test_polygon_area():
    assert abs(polygon_area(np.array([[0, 0], [10, 0], [10, 5], [0, 5.]])) - 50) < 1e-9

def test_measurement_with_true_calibration_within_tolerance():
    rv, tv = measurement_pose(jitter=False); img = render(rv, tv, "measure", seed=2); parts, rms = measure_parts(img, TRUE_K, TRUE_DIST)
    assert rms < 0.2 and len(parts) == 5
    got = sorted([round(p["diameter_mm"], 2) for p in parts if p["shape"] == "circle"]); assert abs(got[0] - 20) < 0.35 and abs(got[1] - 30) < 0.35, got
    big = max([p for p in parts if p["shape"] == "rect"], key=lambda p: p["length_mm"]); assert abs(big["length_mm"] - 90) < 0.35, big

def test_calibration_recovers_focal_length_and_validates():
    r = np.random.RandomState(1); imgs = []
    for i in range(12): rv, tv = random_board_pose(r, 35); imgs.append(render(rv, tv, "board", seed=50 + i))
    cal = calibrate(imgs[:9]); assert abs(cal["K"][0, 0] - 1200) / 1200 < 0.015, cal["K"][0, 0]
    v = validate(cal["K"], cal["dist"], imgs[9:]); assert len(v) == 3 and np.mean(v) < 0.4, v

def test_poor_pose_diversity_is_flagged_by_heldout_error():
    r = np.random.RandomState(2); poor = [render(*random_board_pose(r, 4), "board", seed=70 + i) for i in range(8)]; held = [render(*random_board_pose(r, 35), "board", seed=90 + i) for i in range(4)]
    cal = calibrate(poor); v = validate(cal["K"], cal["dist"], held); assert np.mean(v) > 1.5 * cal["rms"], (np.mean(v), cal["rms"])

if __name__ == "__main__":
    fails = 0
    for name, fn in sorted(globals().items()):
        if name.startswith("test_") and callable(fn):
            try: fn(); print("PASS", name)
            except Exception as e: fails += 1; print("FAIL", name, "->", repr(e))
    sys.exit(1 if fails else 0)

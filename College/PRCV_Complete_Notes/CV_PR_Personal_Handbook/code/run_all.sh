#!/usr/bin/env bash
# Re-creates every figure and prints every measured number quoted in the handbook (about 3-5 minutes).
set -e
cd "$(dirname "$0")"
for f in 01_pr_digits_pipeline 02_bayes_roc_cost 03_segment_and_measure 04_hough_ransac 05_feature_match_homography 06_calibration_synthetic 07_aruco_pose_lab 08_tracking_occlusion_lab 09_diagrams 10_snippets_from_scratch 12_dl_calculators 13_numpy_cnn_digits 14_detection_metrics_calibration; do
  echo "=== $f"; python3 $f.py
done

#!/usr/bin/env python3
"""
Canny edge detector (PC, Python with OpenCV)

Usage:
  python image_canny.py input.jpg [--out edges.png] [--t1 50] [--t2 150] [--l2]

Notes:
  - --t1 and --t2 are the low/high thresholds
  - --l2 enables the more accurate L2 gradient norm
"""
import argparse

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("inp", help="input image path")
    ap.add_argument("--out", default="edges.png", help="output image path")
    ap.add_argument("--t1", type=float, default=50.0, help="low threshold")
    ap.add_argument("--t2", type=float, default=150.0, help="high threshold")
    ap.add_argument("--l2", action="store_true", help="use L2gradient=True")
    args = ap.parse_args()

    try:
        import cv2
    except Exception:
        print("This script requires OpenCV. Install with: pip install opencv-python")
        raise

    img = cv2.imread(args.inp, cv2.IMREAD_GRAYSCALE)
    if img is None:
        raise SystemExit(f"Failed to read image: {args.inp}")

    edges = cv2.Canny(img, threshold1=args.t1, threshold2=args.t2, L2gradient=args.l2)
    cv2.imwrite(args.out, edges)
    print("Saved:", args.out)

if __name__ == "__main__":
    main()

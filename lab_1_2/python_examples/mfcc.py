#!/usr/bin/env python3
"""
MFCC on PC (Python)
- If an input file is given: loads WAV (mono), resamples to --sr (default 16000 Hz)
- Otherwise: generates a 1 s, 440 Hz test tone
- Prints MFCC shape and first frame; can also save a PNG plot

Usage:
  python audio_mfcc.py [path/to/audio.wav] [--sr 16000] [--n_mfcc 13] [--plot out.png]
"""
import argparse
import numpy as np

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("wav", nargs="?", help="input WAV file (optional)")
    ap.add_argument("--sr", type=int, default=16000, help="target sample rate")
    ap.add_argument("--n_mfcc", type=int, default=13, help="number of MFCCs")
    ap.add_argument("--plot", type=str, help="save MFCC plot to this PNG")
    args = ap.parse_args()

    try:
        import librosa, librosa.display  # type: ignore
        import matplotlib.pyplot as plt
    except Exception as e:
        print("This script requires 'librosa' (and matplotlib for --plot). Install:")
        print("  pip install librosa matplotlib")
        raise

    if args.wav:
        y, sr = librosa.load(args.wav, sr=args.sr, mono=True)
        title = f"MFCC: {args.wav} @ {sr} Hz"
    else:
        sr = args.sr
        t = np.arange(0, 1.0, 1/sr)
        y = 0.8*np.sin(2*np.pi*440*t)
        title = f"MFCC: 440 Hz test tone @ {sr} Hz"

    # Typical speech settings
    n_fft = 512
    hop = int(0.010 * sr)   # 10 ms
    win = int(0.025 * sr)   # 25 ms

    mfcc = librosa.feature.mfcc(
        y=y, sr=sr, n_mfcc=args.n_mfcc,
        n_fft=n_fft, hop_length=hop, win_length=win, center=True
    )
    np.set_printoptions(precision=3, suppress=True)
    print("MFCC shape (n_mfcc x frames):", mfcc.shape)
    print("First frame MFCCs:", mfcc[:, 0])

    if args.plot:
        plt.figure()
        librosa.display.specshow(mfcc, x_axis='time', sr=sr, hop_length=hop)
        plt.colorbar(format="%.1f")
        plt.title(title)
        plt.tight_layout()
        plt.savefig(args.plot, dpi=150)
        print("Saved plot:", args.plot)

if __name__ == "__main__":
    main()

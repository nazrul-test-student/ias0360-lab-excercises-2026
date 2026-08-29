import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from math import pi

TEST_MODE = False
TEST_MODE_ADD_NOISE_TO_IMU = True


fs = 2200.0
N = 256
t = np.arange(N) / fs

if TEST_MODE:
    x = 0.7*np.sin(2*pi*120.0*t) + 0.3*np.sin(2*pi*440.0*t)
    if TEST_MODE_ADD_NOISE_TO_IMU:
        imu_noisy = lambda x,t,seed=None: (lambda r,dt: (1+r.normal(0,2e-1))*(x + r.normal(0,2e-3/np.sqrt(2*dt),x.shape) + np.cumsum(r.normal(0,5e-5,x.shape))*dt))(np.random.default_rng(seed), float(np.mean(np.diff(t))))
        x = imu_noisy(x, t, seed=5)
else:
    # Read data from a txt file. (saved from output of mcu)
    with open("raw_imu_mcu.txt", "r") as f:
        data = f.readline()
    x = []
    data = data.split(",")
    for val in data:
        x.append(float(val))
    x = np.array(x)

w = np.hamming(N)
xw = x * w
X = np.fft.rfft(xw, n=N)
mag = np.abs(X)

scale = (2.0/N) / 0.54
amp = mag * scale
freqs = np.fft.rfftfreq(N, 1.0/fs)

K = 5
idx = np.arange(1, len(amp))
top_indices = idx[np.argsort(amp[1:])[::-1][:K]]
top_freqs = freqs[top_indices]
top_amps = amp[top_indices]

order = np.argsort(top_freqs)
top_indices = top_indices[order]
top_freqs = top_freqs[order]
top_amps = top_amps[order]

plt.figure()
plt.plot(t, x)
plt.title(f"Time Signal (fs={fs} Hz, N={N})")
plt.xlabel("Time (s)")
plt.ylabel("Amplitude")
plt.tight_layout()
time_png = "imu_signal.png"
plt.savefig(time_png, dpi=150)

plt.figure()
plt.plot(freqs, amp)
plt.scatter(top_freqs, top_amps, marker='o')
for f, a in zip(top_freqs, top_amps):
    plt.annotate(f"{f:.1f} Hz", xy=(f, a), xytext=(5, 5), textcoords="offset points", fontsize=8)
plt.title("Single-Sided Amplitude Spectrum (Hamming)")
plt.xlabel("Frequency (Hz)")
plt.ylabel("Amplitude (approx)")
plt.xlim(0, fs/2)
plt.tight_layout()
spec_png = "imu_fft_spectrum.png"
plt.savefig(spec_png, dpi=150)

df = pd.DataFrame({"bin": top_indices, "freq_hz": top_freqs, "amplitude": top_amps})
print("Output of FFT on PC")
print(df)


def compare_output(pc_bin, pc_freq, pc_ampli):

    df_mcu = pd.read_csv(
        "out_fft_mcu.txt",
        header=None,
        names=["bin", "mcu_freq", "mcu_amp"],
        skiprows=1,
    ).apply(pd.to_numeric, errors="coerce").dropna()

    df_pc = pd.DataFrame(
        {"bin": pc_bin, "pc_freq": pc_freq, "pc_amp": pc_ampli}
    ).apply(pd.to_numeric, errors="coerce").dropna()

    df = pd.merge(df_mcu, df_pc, on="bin", how="inner", sort=False)

    print("MCU BIN; PC BIN; MCU FREQ; PC FREQ; MCU AMP; PC AMP")
    for _, r in df.iterrows():
        print(f"{r['bin']}; {r['bin']}; {r['mcu_freq']}; {r['pc_freq']}; {r['mcu_amp']}; {r['pc_amp']}")

    is_all_freq_correct = bool((df["mcu_freq"] == df["pc_freq"]).all())
    print("All frequencies match: ", is_all_freq_correct)


if not TEST_MODE:
    compare_output(top_indices, top_freqs, top_amps)
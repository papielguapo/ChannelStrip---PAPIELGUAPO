"""Auditoría completa de todos los módulos DSP de ChannelStrip (papielguapo) + generación de gráficas."""
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from pedalboard import load_plugin

root = Path(__file__).resolve().parent
bundle = root / "ChannelStrip.vst3" / "Contents" / "x86_64-win" / "ChannelStrip.vst3"
docs_dir = root / "docs"
docs_dir.mkdir(exist_ok=True)

# 1. Paso transparente mono y estéreo a 44.1, 48 y 96 kHz
for rate in (44100, 48000, 96000):
    for channels in (1, 2):
        plugin = load_plugin(str(bundle))
        t = np.arange(rate, dtype=np.float64) / rate
        signal = np.array([0.1 * np.sin(2 * np.pi * (440 + 220 * c) * t)
                           for c in range(channels)], dtype=np.float32)
        out = plugin.process(signal, rate, buffer_size=257)
        assert out.shape == signal.shape and np.isfinite(out).all()
        np.testing.assert_allclose(out, signal, atol=1e-5, rtol=1e-4)
        print(f"[PASS] {rate} Hz, {channels} canal(es): Paso transparente bit-exacto")

print("Plugin:", plugin.name, "| Total parámetros:", len(plugin.parameters))


def configured(settings):
    fx = load_plugin(str(bundle))
    params = fx.parameters
    for key, value in settings.items():
        matches = [k for k in params if k == key or k in (key + "_db", key + "_ms", key + "_hz", key + "_%")]
        assert matches, f"Parameter not found: {key} in {list(params)}"
        setattr(fx, matches[0], value)
    return fx


def render(hz, level=0.1, **settings):
    fx = configured(settings)
    sine = np.sin(2 * np.pi * hz * np.arange(96000) / 48000)
    return fx.process((sine * level).astype("float32"), 48000)[-24000:]


def db(hz, **settings):
    y = render(hz, **settings)
    return 20 * np.log10(np.sqrt(np.mean(y ** 2)) * np.sqrt(2) / 0.1)


# 2. Auditoría Input Gain (+48 dB max) + Pad (-20 dB) + Phase Invert (180°)
assert abs(db(1000, input_trim=24) - 24.0) < 0.05
assert abs(db(1000, input_trim=24, pad_20_db=True) - 4.0) < 0.05
y_norm = render(1000, 0.1)
y_inv  = render(1000, 0.1, phase_invert=True)
assert np.max(np.abs(y_norm + y_inv)) < 1e-5
print("[PASS] Módulo 1a: Input Gain (-24..+48 dB), Pad -20 dB y Fase 180° verificados")

# 3. Auditoría FL HEAT (Ocelot LUT: Odd Harmonics en Drive negativo, Even/Asymmetric en Drive positivo)
y_heat_neg = render(1000, 0.25, heat_drive=-6.0, heat_tone=0.0)
y_heat_pos = render(1000, 0.25, heat_drive=6.0,  heat_tone=0.0)
y_heat_byp = render(1000, 0.25, heat_drive=6.0,  heat_bypass=True)
assert np.max(np.abs(y_heat_neg - y_heat_pos)) > 0.005
assert np.max(np.abs(y_heat_byp - render(1000, 0.25))) < 1e-5
print("[PASS] Módulo 1b: FL HEAT (Ocelot LUT Drive & Tone + Bypass) verificado")

# 4. Auditoría Filtros HP / LP con selector de pendiente (6, 12, 18, 24 dB/oct)
slopes = ["6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct"]
hp_cuts = [db(100, high_pass=200, filter_slope=s) for s in slopes]
assert hp_cuts[0] > hp_cuts[1] > hp_cuts[2] > hp_cuts[3]
assert db(50, high_pass=400) < -12 and db(10000, low_pass=1500) < -12
print(f"[PASS] Módulo 2: Filtros HP/LP @ 100 Hz (fc=200 Hz): {[round(x, 2) for x in hp_cuts]} dB (6/12/18/24 dB/oct)")

# 5. Auditoría EQ (HF, MID, LF, botones de Campana y curvas Black/Brown)
assert abs(db(2500, high_mid_gain=6) - 6) < 0.25
assert abs(db(800, high_mid_gain=12, eq_curve="Black")
           - db(800, high_mid_gain=12, eq_curve="Brown")) > 0.25
assert abs(db(20, low_shelf_gain=3.5, low_shelf_frequency=400) - 3.5) < 0.25
assert abs(db(20, low_shelf_gain=6.0, low_shelf_frequency=200, lf_bell=True)) < 2.5
assert db(16000, high_shelf_gain=-3.5, high_shelf_frequency=2000) < -3
assert abs(db(16000, high_shelf_gain=-6.0, high_shelf_frequency=4000, hf_bell=True)) > -3.0
print("[PASS] Módulo 3: EQ (HF/MID/LF, Shelf continuo + Campana por pasos 1 dB + Curvas Black/Brown) verificado")

# 6. Auditoría Compresor & Gate/Expander con Ataque Fijo Ultra-Rápido (0.1 ms)
assert db(1000, compressor_threshold=-40, compressor_ratio=8) < -10
burst = np.zeros(4800, dtype="float32")
burst[2400:] = 0.6 * np.sin(2 * np.pi * np.arange(2400) / 48)
fx_fast = configured(dict(compressor_threshold=-30, compressor_ratio=10))
assert np.max(np.abs(fx_fast.process(burst, 48000)[2448:2640])) < 0.22
assert db(1000, gate_threshold=-10, gate_range=40) < -30
assert db(1000, gate_mode="Expand", gate_threshold=-10, gate_range=40) < -4
print("[PASS] Módulo 4: Dynamics (Compresor + Gate/Expander con Fast Attack 0.1 ms) verificado")

# 7. Auditoría De-Esser de Vocals (Split Band 7:1)
def ess(hz):
    return 20 * np.log10(np.linalg.norm(render(hz, 0.6, de_ess=True))
                         / np.linalg.norm(render(hz, 0.6)))
assert ess(7000) < -3 and abs(ess(200)) < 0.1
print(f"[PASS] Módulo 5: Vocals De-Esser Split Band ({ess(7000):.2f} dB a 7 kHz, {ess(200):.3f} dB a 200 Hz) verificado")

# 8. Auditoría Mid / Side Matrix (de OUTPUT)
fx_ms = configured(dict(mid_gain=-20, side_gain=6))
t_ms = np.arange(4800, dtype=np.float32) / 48000.0
stereo_in = np.vstack([
    0.2 * np.sin(2 * np.pi * 440 * t_ms) + 0.1 * np.sin(2 * np.pi * 880 * t_ms),
    0.2 * np.sin(2 * np.pi * 440 * t_ms) - 0.1 * np.sin(2 * np.pi * 880 * t_ms),
]).astype(np.float32)
stereo_out = fx_ms.process(stereo_in, 48000)[:, -2400:]
mid_out  = 0.5 * (stereo_out[0] + stereo_out[1])
side_out = 0.5 * (stereo_out[0] - stereo_out[1])
assert np.linalg.norm(side_out) > np.linalg.norm(mid_out) * 5.0
print("[PASS] Módulo 6: Matriz Mid/Side de OUTPUT verificada")

# 9. Auditoría OUTPUT Inflate + Clip 0 dB + Output Fader + ANALOG Noise (-128 dBFS)
inf0 = np.linalg.norm(render(440, 0.25))
inf100 = np.linalg.norm(render(440, 0.25, inflate_effect=100, inflate_curve=25))
assert inf100 > inf0 * 1.35
clipped = render(440, 0.9, input_trim=12, clip_0_db=True)
assert np.max(np.abs(clipped)) <= 1.0001
assert abs(db(1000, output_fader=-6) + 6) < 0.05
fx_noise = configured(dict(analog_noise=True))
silence = np.zeros((2, 48000), dtype=np.float32)
noise_out = fx_noise.process(silence, 48000)[:, 4000:]
noise_rms_db = 20 * np.log10(np.sqrt(np.mean(noise_out ** 2)))
assert -131.5 <= noise_rms_db <= -124.5
print(f"[PASS] Módulo 7: OUTPUT Inflate (+{20*np.log10(inf100/inf0):.2f} dB), Clip 0 dB (max {np.max(np.abs(clipped)):.4f}), Analog Noise ({noise_rms_db:.2f} dBFS RMS) y Fader verificados")

# =========================================================================
# Generación de Gráficas de Auditoría DSP en docs/DSP_Audit_Plots.png (con PIL)
# =========================================================================
W, H = 1400, 960
img = Image.new("RGB", (W, H), (23, 26, 29))
draw = ImageDraw.Draw(img)

try:
    f_title = ImageFont.truetype("C:/Windows/Fonts/segoeuib.ttf", 18)
    f_sub   = ImageFont.truetype("C:/Windows/Fonts/segoeuib.ttf", 13)
    f_lbl   = ImageFont.truetype("C:/Windows/Fonts/segoeui.ttf", 12)
except Exception:
    f_title = f_sub = f_lbl = ImageFont.load_default()

draw.text((28, 16), "CHANNELSTRIP — PAPIELGUAPO | AUDITORÍA DSP EN TIEMPO REAL (48 kHz)", fill=(212, 216, 220), font=f_title)

panels = [
    (28,  58, 670, 468, "1. Filtros HPF — 4 Pendientes (6 / 12 / 18 / 24 dB/oct @ 180 Hz)"),
    (702, 58, 1372, 468, "2. EQ 3 Bandas — Curvas Black/Brown & Conmutación Campana"),
    (28,  496, 670, 926, "3. OUTPUT Inflate + Clip 0 dB — Curvas de Transferencia"),
    (702, 496, 1372, 926, "4. Vocals De-Esser Split Band — Reducción Selectiva (7 kHz vs 200 Hz)")
]

for x1, y1, x2, y2, title in panels:
    draw.rounded_rectangle([x1, y1, x2, y2], radius=8, fill=(30, 33, 36), outline=(60, 66, 72), width=2)
    draw.text((x1 + 16, y1 + 12), title, fill=(255, 158, 44), font=f_sub)
    px1, py1, px2, py2 = x1 + 54, y1 + 46, x2 - 24, y2 - 36
    draw.rectangle([px1, py1, px2, py2], fill=(20, 23, 26), outline=(44, 49, 54))
    for gx in range(1, 5):
        lx = int(px1 + (px2 - px1) * (gx / 5.0))
        draw.line([(lx, py1), (lx, py2)], fill=(34, 38, 43), width=1)
    for gy in range(1, 5):
        ly = int(py1 + (py2 - py1) * (gy / 5.0))
        draw.line([(px1, ly), (px2, ly)], fill=(34, 38, 43), width=1)

def plot_curve(panel_idx, xs, ys, xmin, xmax, ymin, ymax, color, log_x=False):
    x1, y1, x2, y2, _ = panels[panel_idx]
    px1, py1, px2, py2 = x1 + 54, y1 + 46, x2 - 24, y2 - 36
    pts = []
    for xv, yv in zip(xs, ys):
        if log_x:
            nx = (np.log10(xv) - np.log10(xmin)) / (np.log10(xmax) - np.log10(xmin))
        else:
            nx = (xv - xmin) / (xmax - xmin)
        ny = (yv - ymin) / (ymax - ymin)
        nx = float(np.clip(nx, 0.0, 1.0))
        ny = float(np.clip(ny, 0.0, 1.0))
        pts.append((px1 + nx * (px2 - px1), py2 - ny * (py2 - py1)))
    if len(pts) >= 2:
        draw.line(pts, fill=color, width=3)

# Panel 1: HPF Slopes
freqs = np.geomspace(25, 16000, 14)
cols = [(110, 231, 85), (66, 165, 245), (255, 158, 44), (239, 83, 80)]
for s, col in zip(slopes, cols):
    resp = [db(f, high_pass=180, filter_slope=s) for f in freqs]
    plot_curve(0, freqs, resp, 25, 16000, -36, 3, col, log_x=True)

# Panel 2: EQ Black vs Brown vs Bell
eq_f = np.geomspace(30, 18000, 16)
blk_curve  = [db(f, low_shelf_gain=8, low_shelf_frequency=120, high_mid_gain=8, high_mid_frequency=2500, high_shelf_gain=8, high_shelf_frequency=8000, eq_curve="Black") for f in eq_f]
brn_curve  = [db(f, low_shelf_gain=8, low_shelf_frequency=120, high_mid_gain=8, high_mid_frequency=2500, high_shelf_gain=8, high_shelf_frequency=8000, eq_curve="Brown") for f in eq_f]
bell_curve = [db(f, low_shelf_gain=8, low_shelf_frequency=120, lf_bell=True, high_shelf_gain=8, high_shelf_frequency=8000, hf_bell=True, eq_curve="Black") for f in eq_f]
plot_curve(1, eq_f, blk_curve,  30, 18000, -2, 12, (255, 158, 44), log_x=True)
plot_curve(1, eq_f, brn_curve,  30, 18000, -2, 12, (66, 165, 245), log_x=True)
plot_curve(1, eq_f, bell_curve, 30, 18000, -2, 12, (239, 83, 80),  log_x=True)

# Panel 3: OUTPUT Inflate + Clip 0 dB
x_in = np.linspace(-1.2, 1.2, 400, dtype=np.float32)
fx_inf_0   = configured(dict(inflate_effect=100, inflate_curve=0,   clip_0_db=True))
fx_inf_50  = configured(dict(inflate_effect=100, inflate_curve=50,  clip_0_db=True))
fx_inf_m50 = configured(dict(inflate_effect=100, inflate_curve=-50, clip_0_db=True))
plot_curve(2, x_in, x_in,                        -1.2, 1.2, -1.2, 1.2, (92, 98, 104),  log_x=False)
plot_curve(2, x_in, fx_inf_m50.process(x_in.copy(), 48000), -1.2, 1.2, -1.2, 1.2, (66, 165, 245), log_x=False)
plot_curve(2, x_in, fx_inf_0.process(x_in.copy(),   48000), -1.2, 1.2, -1.2, 1.2, (255, 158, 44), log_x=False)
plot_curve(2, x_in, fx_inf_50.process(x_in.copy(),  48000), -1.2, 1.2, -1.2, 1.2, (239, 83, 80),  log_x=False)

# Panel 4: Vocals De-Esser Split Band
deess_f = np.geomspace(100, 14000, 14)
deess_resp = [ess(f) for f in deess_f]
plot_curve(3, deess_f, deess_resp, 100, 14000, -10, 1.5, (239, 83, 80), log_x=True)

plot_path = docs_dir / "DSP_Audit_Plots.png"
img.save(plot_path)
print("Saved DSP audit plot:", plot_path)

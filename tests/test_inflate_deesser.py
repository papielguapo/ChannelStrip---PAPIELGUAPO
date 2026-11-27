from pathlib import Path
from pedalboard import load_plugin
import numpy as np
from pathlib import Path
path=str(Path(__file__).resolve().parents[1] / 'build/SurcoE_artefacts/Release/VST3/ChannelStrip.vst3/Contents/x86_64-win/ChannelStrip.vst3')
def render(x,rate=48000,**settings):
 p=load_plugin(path)
 for k,v in settings.items():setattr(p,k,v)
 return p.process(np.asarray(x,dtype='float32'),rate,buffer_size=257)
rate=48000; t=np.arange(24000)/rate
for amp in (.5,1,2,4,16):
 x=(amp*np.sin(2*np.pi*1000*t)).astype('float32')
 dry=render(x,inflate_effect=0);wet=render(x,inflate_effect=100)
 for effect in (.01,1,25,50):
  y=render(x,inflate_effect=effect)
  expected=dry+(wet-dry)*(effect/100)
  assert np.allclose(y,expected,atol=4e-6), (amp,effect,np.max(abs(y-expected)))
 clip=render(x,inflate_effect=1,clip_0_db=True)
 assert max(abs(clip))<=1.000001
 print('PASS INFLATE continuous mix and explicit CLIP at input peak',amp)
for sr in (44100,48000,96000):
 time=np.arange(sr//2)/sr
 for f in (200,3000,6200,10000):
  x=(.001*np.sin(2*np.pi*f*time)).astype('float32')
  y=render(x,sr,deesser_freq_hz=6200,deesser_thresh_db=-6)
  assert np.allclose(x,y,atol=1e-7), (sr,f)
 print('PASS de-esser transparent without gain reduction at',sr)
# SPLIT attenuates sibilance, retaining the lower voice band; stereo detector is linked.
def tone(f):return (.3*np.sin(2*np.pi*f*t)).astype('float32')
def db_ratio(a,b):return 20*np.log10(np.linalg.norm(a[-12000:])/np.linalg.norm(b[-12000:]))
low=tone(200);high=tone(8000)
a=render(low,deesser_freq_hz=6200,deesser_thresh_db=-30)
b=render(high,deesser_freq_hz=6200,deesser_thresh_db=-30)
assert abs(db_ratio(a,low))<.3
assert db_ratio(b,high)<-3
print('PASS de-esser split low/high gain dB',db_ratio(a,low),db_ratio(b,high))
x=np.stack([high,high*.5]); y=render(x,deesser_freq_hz=6200,deesser_thresh_db=-30)
assert np.allclose(y[0],2*y[1],atol=1e-6)
ywide=render(np.stack([low,high]),deesser_freq_hz=6200,deesser_thresh_db=-30,deesser_audio_mode='Wide')
assert db_ratio(ywide[0],low)<-3
print('PASS stereo-linked detector and WIDE reduction')
# Bypass release reaches the exact dry signal instead of leaving a tonal notch.
p=load_plugin(path);p.deesser_thresh_db=-30;p.deesser_freq_hz=6200
p.process(high,48000);p.de_ess=False
out=p.process(high,48000,reset=False)
assert np.allclose(out[-12000:],high[-12000:],atol=1e-6)
print('PASS de-esser smooth bypass returns to dry')

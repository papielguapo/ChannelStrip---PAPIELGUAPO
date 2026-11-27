from pedalboard import load_plugin
import numpy as np
from pathlib import Path
path=str(Path(__file__).resolve().parents[1] / 'build/SurcoE_artefacts/Release/VST3/ChannelStrip.vst3/Contents/x86_64-win/ChannelStrip.vst3')
sr=48000;t=np.arange(sr)/sr
# Both 80 Hz voice fundamentals and HF signals must hold the gate open.
def process(x,gain=100,release=50):
 p=load_plugin(path);p.gate_gain=gain;p.gate_threshold_db=-30;p.gate_release_ms=release
 return p.process(x.astype('float32'),sr,buffer_size=257)
for freq in (80,1000,8000):
 x=.2*np.sin(2*np.pi*freq*t)
 y=process(x,0)
 assert np.allclose(y[-24000:],x[-24000:],atol=1e-6),(freq,max(abs(y[-24000:]-x[-24000:])))
 print('PASS gate holds open across zero crossings',freq)
x=.001*np.sin(2*np.pi*1000*t)
for gain in (100,50,10,0):
 y=process(x,gain)
 ratio=np.linalg.norm(y[-12000:])/np.linalg.norm(x[-12000:])
 assert abs(ratio-gain/100)<1e-5,(gain,ratio)
 print('PASS closed GAIN',gain,'measured',ratio)
# Release controls the time to close after a loud passage.
x=np.where(t<.2,.2,.001)*np.sin(2*np.pi*1000*t)
fast=process(x,0,10);slow=process(x,0,500)
region=slice(18000,20000)
assert np.linalg.norm(slow[region])>np.linalg.norm(fast[region])*10
print('PASS release shapes closing tail')
x=np.stack([.2*np.sin(2*np.pi*1000*t),.001*np.sin(2*np.pi*1000*t)])
y=process(x,0)
assert np.allclose(y[:,-24000:],x[:,-24000:],atol=1e-6)
print('PASS stereo linked gate does not pan')

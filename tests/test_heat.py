from pedalboard import load_plugin
import numpy as np
from pathlib import Path
path=str(Path(__file__).resolve().parents[1] / 'build/SurcoE_artefacts/Release/VST3/ChannelStrip.vst3/Contents/x86_64-win/ChannelStrip.vst3')
p=load_plugin(path)
print('OS choices:',p.parameters['oversampling'].valid_values)
for mode in p.parameters['oversampling'].valid_values:
 p=load_plugin(path); p.oversampling=mode
 x=np.zeros((2,8192),dtype=np.float32); x[:,1000]=0.1
 y=p.process(x,48000,buffer_size=128)
 print(mode,'latency',p.reported_latency_samples,'finite',np.isfinite(y).all())
 assert np.isfinite(y).all()
 assert (p.reported_latency_samples==0) == ('1x' in mode)
p=load_plugin(path)
x=np.random.default_rng(1).normal(0,.03,(2,4096)).astype('float32')
y=p.process(x,48000,buffer_size=257)
assert np.allclose(x,y,atol=1e-5)
print('PASS native transparency')
p=load_plugin(path); p.heat_drive=6
p.process(x,48000,buffer_size=128)
p.heat_drive=0
out=p.process(np.full((2,256),.1,dtype='float32'),48000,buffer_size=128,reset=False)
assert np.isfinite(out).all()
assert abs(float(out[0,0])-.1)>1e-5
print('PASS drive-zero smoothing remains active')

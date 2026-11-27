from pedalboard import load_plugin
import numpy as np
from pathlib import Path
path=str(Path(__file__).resolve().parents[1] / 'build/SurcoE_artefacts/Release/VST3/ChannelStrip.vst3/Contents/x86_64-win/ChannelStrip.vst3')
rng=np.random.default_rng(4)
x=rng.normal(0,.05,(2,24000)).astype('float32')
def run(x,sep=100,mono=False):
 p=load_plugin(path); p.stereo_width=sep; p.mono=mono
 return p.process(x,48000,buffer_size=257)
assert np.allclose(run(x),x,atol=1e-6)
mid=x.mean(axis=0); side=(x[0]-x[1])*.5
for sep in (0,50,100,150,200):
 y=run(x,sep); scale=sep/100
 expected=np.stack([mid+side*scale,mid-side*scale])
 assert np.allclose(y,expected,atol=1e-6)
 assert np.allclose(run(x[::-1],sep),y[::-1],atol=1e-6)
 print('PASS matrix / channel symmetry',sep)
mono=np.tile(mid,(2,1))
assert np.allclose(run(mono,200),mono,atol=1e-6)
assert np.allclose(run(x,mono=True),mono,atol=1e-6)
# Left-only input: instantaneous mixing, no echoes / delayed comb response.
imp=np.zeros((2,8192),dtype='float32'); imp[0,1000]=.1
out=run(imp,200)
assert np.count_nonzero(np.abs(out)>1e-7)==2
print('PASS true MONO button, mono source unchanged, no delayed impulse or comb filtering')

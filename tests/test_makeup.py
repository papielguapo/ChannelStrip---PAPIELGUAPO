from pedalboard import load_plugin
import numpy as np
from pathlib import Path
path=str(Path(__file__).resolve().parents[1] / 'build/SurcoE_artefacts/Release/VST3/ChannelStrip.vst3/Contents/x86_64-win/ChannelStrip.vst3')
x=np.tile((.15*np.sin(2*np.pi*1000*np.arange(48000)/48000)).astype('float32'),(2,1))
def run(on,ratio=4,comp=True,dyn=True,thr=-24):
 p=load_plugin(path); assert p.auto_makeup == False
 p.compressor_threshold_db=thr; p.compressor_ratio=ratio; p.compressor_in=comp; p.dynamics_in=dyn; p.auto_makeup=on
 y=p.process(x,48000,buffer_size=128)
 assert np.isfinite(y).all()
 return y
for ratio,comp,dyn in [(1,True,True),(4,False,True),(4,True,False)]:
 assert np.allclose(run(True,ratio,comp,dyn),run(False,ratio,comp,dyn),atol=1e-7)
 print('PASS no boost when compressor inactive',ratio,comp,dyn)
for thr,expected in [(-24,9),(-60,18)]:
 a=run(False,thr=thr); b=run(True,thr=thr)
 boost=20*np.log10(np.sqrt(np.mean(b[:,-12000:]**2))/np.sqrt(np.mean(a[:,-12000:]**2)))
 assert abs(boost-expected)<.02,(boost,expected)
 print('PASS makeup boost dB:',float(boost))
p=load_plugin(path); p.compressor_threshold_db=-24; p.compressor_ratio=4
p.process(x,48000,buffer_size=128); p.auto_makeup=True
on=p.process(x,48000,buffer_size=128,reset=False); p.auto_makeup=False
off=p.process(x,48000,buffer_size=128,reset=False)
assert np.max(np.abs(np.diff(on[0])))<.08
assert np.allclose(off[:,-12000:],run(False)[:,-12000:],atol=1e-5)
print('PASS smooth toggle and return to manual gain')


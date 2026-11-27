from pedalboard import load_plugin
import numpy as np
from pathlib import Path
path=str(Path(__file__).resolve().parents[1] / 'build/SurcoE_artefacts/Release/VST3/ChannelStrip.vst3/Contents/x86_64-win/ChannelStrip.vst3')
def measure(level,knee,ratio=4,mode='Peak'):
 p=load_plugin(path);p.compressor_threshold_db=-24;p.compressor_ratio=ratio;p.compressor_knee_db=knee;p.compressor_detector=mode
 amp=10**(level/20);x=np.full(12000,amp,dtype='float32')
 y=p.process(x,48000,buffer_size=128)
 return -20*np.log10(np.mean(y[-2000:])/amp)
for knee in (0,6,12,24):
 vals=[]
 for level in (-40,-30,-24,-18,-6):
  over=level+24
  shape=((over+knee/2)**2/(2*knee)) if knee>0 and -knee/2<over<knee/2 else max(0,over)
  expected=shape*.75
  measured=measure(level,knee)
  assert abs(measured-expected)<.03,(level,knee,expected,measured)
  vals.append(round(float(measured),3))
 print('PASS knee',knee,'dB reduction',vals)
assert abs(measure(-24,12,1))<.001
assert abs(measure(-24,12,4,'RMS')-1.125)<.04
print('PASS ratio 1 bypass and RMS knee')


# Copyright 2016 Brian Innes
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import sys
import traceback
from vPiP import *
from vPiP.renderers.spiralArcRenderer import renderSpiralArc
import time
import math
from subprocess import *

Vpip = vPiP.Vpip

width = 200
height = width/4*3
numwide = int(math.floor(1000.0/width))
numhigh = int(math.floor(1000.0/height))

for x in range(1,numwide,1):
  for y in range(numhigh):
    if x <= 1 and y <= 1:
      continue
    call(["bash", "get-new-image.sh"])
    filename = check_output(["bash","get-latest.sh"])
    filename = filename.decode('utf-8').strip()
#    print filename
    filename = "/home/pi/photos/"+filename
#    filename = "../testImages/TyneBridge.jpg"
#    filename = "../testImages/SydneyOpera.jpg"
#    filename = "../testImages/SydneyOperaNight.jpg"
#    filename = "../testImages/HamptonCourt.jpg"
    with Vpip() as p:
#      p.setShowDrawing(True)
#      p.setPlotting(False)
      try:
#        p.moveTo(0,0)
#        p.drawTo(0,0)
        renderSpiralArc(filename, x*width, y*height, width, 4, p)
#        renderSpiralArc(filename, 100, 160, 80, 2, p)
#        renderSpiralArc(filename, 200, 1000, 800, 15, p)
#        renderSpiralArc(filename, 0, 1950, 1200, 20, p)
#        renderSpiralArc(filename, 50, 100, 500, 25, p)
#        p.goHome()
      except:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        print("test1 main thread exception : %s" % exc_type)
        traceback.print_tb(exc_traceback, limit=2, file=sys.stdout)

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
from vPiP.renderers.norwegianSpiral import renderNorwegianSpiral
Vpip = vPiP.Vpip

filename = "/home/pi/vPiP/testImages/HamptonCourt.jpg"
#filename = "../testImages/TyneBridge.jpg"
#filename = "../testImages/SydneyOpera.jpg"
#filename = "../testImages/SydneyOperaNight.jpg"
#filename = "../testImages/HamptonCourt.jpg"
with Vpip() as p:
#    p.setShowDrawing(True)
#    p.setPlotting(False)
    try:
        p.moveTo(250,250)
        p.goHome()
    except:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        print("test3 main thread exception : %s" % exc_type)
        traceback.print_tb(exc_traceback, limit=2, file=sys.stdout)

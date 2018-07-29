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
from vPiP.vPiP import Vpip, ConstrainDrawingRectangle
from vPiP.renderers.svg import renderSVG


filename = "/home/pi/vPiP/testImages/Vulcan.svg"

with Vpip() as p:
#    p.setShowDrawing(True)
#    p.setPlotting(False)
    try:
        renderSVG(filename, 300, 200, 600, p)
        renderSVG(filename, 200, 1000, 800, p)
        renderSVG(filename, 0, 1950, 1200, p)
        d = ConstrainDrawingRectangle(1200, 0, 5000, p.height, p)
        renderSVG(filename, 1200, 0, 3800, d, True)
        p.goHome()
    except:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        print("test1 main thread exception : %s" % exc_type)
        traceback.print_tb(exc_traceback, limit=2, file=sys.stdout)

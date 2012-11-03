#!/usr/bin/env python2
# -*- coding: utf-8 -*-
#
# Webcamod, webcam capture plasmoid.
# Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
#
# Webcamod is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Webcamod is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
#
# Email     : hipersayan DOT x AT gmail DOT com
# Web-Site 1: http://github.com/hipersayanX/Webcamoid
# Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796

import os
import re
import sys
import threading

from PyQt4 import QtCore, QtGui


class Pipeline(QtCore.QObject):
    oStream = QtCore.pyqtSignal(QtGui.QImage, str)
    playingStateChanged = QtCore.pyqtSignal(bool)

    def __init__(self, pipeline='', blocking=True, parent=None, debug=False):
        QtCore.QObject.__init__(self, parent)

        self.pipeline = pipeline
        self.blocking = blocking
        self.debug = debug

        self.pipelineObject = None
        self.playing = False

        self.gobject = None
        self.gtk = None
        self.pygst = None
        self.gst = None

        self.mutex = QtCore.QMutex()

    def on_message(self, bus, message):
        if message.type == self.gst.MESSAGE_EOS:
            self.pipelineObject.set_state(self.gst.STATE_NULL)
        elif message.type == self.gst.MESSAGE_ERROR:
            self.pipelineObject.set_state(self.gst.STATE_NULL)

            err, debug = message.parse_error()
            print('Error: {0} {1}'.format(err, debug))

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.stop()

        return True

    def initGst(self):
        if not self.gobject:
            try:
                self.gobject = __import__('gobject')
            except:
                return False

        if not self.pygst:
            try:
                self.pygst = __import__('pygst')
                self.pygst.require('0.10')
            except:
                return False

        if not self.gst:
            try:
                self.gst = __import__('gst')
                self.gobject.threads_init()
            except:
                return False

        return True

    def availableElements(self):
        self.initGst()
        registry = self.gst.registry_get_default()

        for elementFactory in registry.get_feature_list(self.gst.ElementFactory):
            yield (elementFactory.get_name(), elementFactory.get_klass())

    def hashFromName(self, name='', type='x'):
        return type + name.encode('hex')

    def nameFromHash(self, hash=''):
        return hash[1:].decode('hex')

    @QtCore.pyqtSlot()
    def start(self):
        self.stop()

        if self.pipeline == '':
            return

        if not self.initGst():
            return

        try:
            pipeline = []
            elementType = ''
            splitedParam = []
            sinks = []
            bracketMatch = False
            quoteMatch = False

            for param in self.pipeline.split():
                if param.startswith('name={') and not quoteMatch:
                    if param.endswith('}'):
                        pipename = re.findall('name={(.+)}', param)

                        if pipename != []:
                            type = 'i' if elementType == 'appsrc' else 'o'
                            hash = self.hashFromName(pipename[0], type)
                            pipeline.append('name={0}'.format(hash))

                            if elementType == 'appsink':
                                sinks.append(hash)

                            elementType = ''
                    else:
                        splitedParam.append(param)
                        bracketMatch = True
                elif param.endswith('}') and not quoteMatch:
                    splitedParam.append(param)
                    param = ' '.join(splitedParam)
                    pipename = re.findall('name={(.+)}', param)

                    if pipename != []:
                        type = 'i' if elementType == 'appsrc' else 'o'
                        hash = self.hashFromName(pipename[0], type)
                        pipeline.append('name={0}'.format(hash))

                        if elementType == 'appsink':
                            sinks.append(hash)

                        elementType = ''

                    splitedParam = []
                    bracketMatch = False
                elif re.match('^[a-z](?:-?[a-z]+)*="', param) and not bracketMatch:
                    if param.endswith('"'):
                        pipeline.append(param)
                    else:
                        splitedParam.append(param)
                        quoteMatch = True
                elif param.endswith('"') and not bracketMatch:
                    splitedParam.append(param)
                    pipeline.append(' '.join(splitedParam))
                    splitedParam = []
                    quoteMatch = False
                elif param == 'appsrc' and not bracketMatch and not quoteMatch:
                    elementType = param
                    pipeline.append(param)
                elif param == 'appsink' and not bracketMatch and not quoteMatch:
                    elementType = param
                    pipeline.append(param)
                else:
                    if splitedParam == []:
                        pipeline.append(param)
                    else:
                        splitedParam.append(param)

            self.pipelineObject = self.gst.parse_launch(' '.join(pipeline))

            for sinkName in sinks:
                sink = self.pipelineObject.get_by_name(sinkName)
                sink.connect('new-buffer', self.readData)

            self.bus = self.pipelineObject.get_bus()
            self.bus.add_signal_watch()
            self.bus.connect('message', self.on_message)

            self.pipelineObject.set_state(self.gst.STATE_PLAYING)

            self.playing = True
            self.playingStateChanged.emit(True)
        except:
            self.pipelineObject = None

    @QtCore.pyqtSlot()
    def stop(self):
        if not self.playing:
            return

        self.pipelineObject.send_event(self.gst.event_new_eos())
        self.pipelineObject.set_state(self.gst.STATE_NULL)
        self.bus.remove_signal_watch()
        self.pipelineObject = None
        self.playing = False
        self.playingStateChanged.emit(False)
        self.srcs = []
        self.sinks = []

    @QtCore.pyqtSlot(QtGui.QImage, str)
    def iStream(self, frame, pipename):
        pipename = str(pipename)

        if not self.playing:
            if not self.blocking:
                self.oStream.emit(frame, pipename)

            return

        srcHash = self.hashFromName(pipename, 'i')
        src = self.pipelineObject.get_by_name(srcHash)

        data = QtCore.QBuffer()
        data.open(QtCore.QIODevice.ReadWrite)
        frame.save(data, 'BMP')
        buffer = self.gst.Buffer(data.data().data())
        buffer.set_caps(self.gst.caps_from_string('image/bmp'))
        src.emit('push-buffer', buffer)
        data.close()

    @QtCore.pyqtSlot(list)
    def setPipeline(self, pipeline):
        playing = self.playing
        self.stop()
        self.pipeline = pipeline

        if playing:
            self.start()

    @QtCore.pyqtSlot(bool)
    def setBlocking(self, blocking):
        self.blocking = blocking

    def readData(self, sink):
        self.mutex.lock()
        pipename = self.nameFromHash(sink.get_name())
        frame = sink.emit('pull-buffer')
        self.oStream.emit(QtGui.QImage.fromData(frame), pipename)
        self.mutex.unlock()


if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    pipeline = Pipeline('v4l2src device=/dev/video0 ! video/x-raw-yuv,width=640,height=480 ! ' \
                        'ffmpegcolorspace ! ffenc_bmp ! appsink name={capture}')

    @QtCore.pyqtSlot(str, str)
    def dataRec(data, pipename):
        print(pipename)

    pipeline.oStream.connect(dataRec)
    pipeline.start()
    QtCore.QTimer.singleShot(5000, pipeline.stop)

    app.exec_()

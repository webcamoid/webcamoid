# Tests for Python bindings for the v4l2 userspace api

# Copyright (C) 1999-2009 the contributors

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# Alternatively you can redistribute this file under the terms of the
# BSD license as stated below:

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. The names of its contributors may not be used to endorse or promote
#    products derived from this software without specific prior written
#    permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""
Tests for Python bindings for the v4l2 userspace api
"""

import v4l2
import errno
from fcntl import ioctl


#
# validators
#

def valid_string(string):
    for char in string:
        if (ord(char) < 32 or 126 < ord(char)):
            return False
    return True


def valid_v4l2_std_id(std_id):
    return std_id & ~ (
        v4l2.V4L2_STD_PAL_B |
        v4l2.V4L2_STD_PAL_B1 |
        v4l2.V4L2_STD_PAL_G |
        v4l2.V4L2_STD_PAL_H |
        v4l2.V4L2_STD_PAL_I |
        v4l2.V4L2_STD_PAL_D |
        v4l2.V4L2_STD_PAL_D1 |
        v4l2.V4L2_STD_PAL_K |
        v4l2.V4L2_STD_PAL_M |
        v4l2.V4L2_STD_PAL_N |
        v4l2.V4L2_STD_PAL_Nc |
        v4l2.V4L2_STD_PAL_60 |
        v4l2.V4L2_STD_NTSC_M |
        v4l2.V4L2_STD_NTSC_M_JP |
        v4l2.V4L2_STD_NTSC_443 |
        v4l2.V4L2_STD_NTSC_M_KR |
        v4l2.V4L2_STD_SECAM_B |
        v4l2.V4L2_STD_SECAM_D |
        v4l2.V4L2_STD_SECAM_G |
        v4l2.V4L2_STD_SECAM_H |
        v4l2.V4L2_STD_SECAM_K |
        v4l2.V4L2_STD_SECAM_K1 |
        v4l2.V4L2_STD_SECAM_L |
        v4l2.V4L2_STD_SECAM_LC |
        v4l2.V4L2_STD_ATSC_8_VSB |
        v4l2.V4L2_STD_ATSC_16_VSB) == 0


def valid_capabilities(capabilities):
    return capabilities & ~ (
        v4l2.V4L2_CAP_VIDEO_CAPTURE |
        v4l2.V4L2_CAP_VIDEO_OUTPUT |
        v4l2.V4L2_CAP_VIDEO_OVERLAY |
        v4l2.V4L2_CAP_VBI_CAPTURE |
        v4l2.V4L2_CAP_VBI_OUTPUT |
        v4l2.V4L2_CAP_SLICED_VBI_CAPTURE |
        v4l2.V4L2_CAP_SLICED_VBI_OUTPUT |
        v4l2.V4L2_CAP_RDS_CAPTURE |
        v4l2.V4L2_CAP_VIDEO_OUTPUT_OVERLAY |
        v4l2.V4L2_CAP_TUNER |
        v4l2.V4L2_CAP_AUDIO |
        v4l2.V4L2_CAP_RADIO |
        v4l2.V4L2_CAP_READWRITE |
        v4l2.V4L2_CAP_ASYNCIO |
        v4l2.V4L2_CAP_STREAMING) == 0


def valid_input_status(status):
    return status & (
        v4l2.V4L2_IN_ST_NO_POWER |
        v4l2.V4L2_IN_ST_NO_SIGNAL |
        v4l2.V4L2_IN_ST_NO_COLOR |
        v4l2.V4L2_IN_ST_NO_H_LOCK |
        v4l2.V4L2_IN_ST_COLOR_KILL |
        v4l2.V4L2_IN_ST_NO_SYNC |
        v4l2.V4L2_IN_ST_NO_EQU |
        v4l2.V4L2_IN_ST_NO_CARRIER |
        v4l2.V4L2_IN_ST_MACROVISION |
        v4l2.V4L2_IN_ST_NO_ACCESS |
        v4l2.V4L2_IN_ST_VTR)


#
# helpers
#

def get_device_inputs(fd):
    index = 0
    while True:
        input_ = v4l2.v4l2_input(index)
        try:
            ioctl(fd, v4l2.VIDIOC_ENUMINPUT, input_)
        except IOError, e:
            assert e.errno == errno.EINVAL
            break
        yield input_
        index += 1


def get_device_outputs(fd):
    index = 0
    while True:
        output = v4l2.v4l2_output(index)
        try:
            ioctl(fd, v4l2.VIDIOC_ENUMOUTPUT, output)
        except IOError, e:
            assert e.errno == errno.EINVAL
            break
        yield output
        index += 1


def foreach_device_input(fd, func):
    original_index = v4l2.c_int()
    ioctl(fd, v4l2.VIDIOC_G_INPUT, original_index)

    for input_ in get_device_inputs(fd):
        if input_.index != original_index.value:
            try:
                ioctl(fd, v4l2.VIDIOC_S_INPUT, v4l2.c_int(input_.index))
            except IOError, e:
                if e.errno == errno.EBUSY:
                    continue
                else:
                    raise
        func(fd, input_)

    try:
        ioctl(fd, v4l2.VIDIOC_S_INPUT, original_index)
    except IOError, e:
        if not (e.errno == errno.EBUSY):
            raise


def foreach_device_output(fd, func):
    original_index = v4l2.c_int()
    ioctl(fd, v4l2.VIDIOC_G_OUTPUT, original_index)

    for output in get_device_outputs(fd):
        if output_.index != original_index.value:
            try:
                ioctl(fd, v4l2.VIDIOC_S_OUTPUT, v4l2.c_int(output.index))
            except IOError, e:
                if e.errno == errno.EBUSY:
                    continue
                else:
                    raise
        func(fd, output)

    try:
        ioctl(fd, v4l2.VIDIOC_S_INPUT, original_index)
    except IOError, e:
        if not (e.errno == errno.EBUSY):
            raise


def get_device_standards(fd):
    # Note that according to the spec this is input/output specific,
    # and so the yielded standards reflect the active input/output.
    index = 0
    while True:
        std = v4l2.v4l2_standard(index)
        try:
            ioctl(fd, v4l2.VIDIOC_ENUMSTD, std)
        except IOError, e:
            assert e.errno == errno.EINVAL
            break
        yield std
        index += 1


def get_device_controls(fd):
    # original enumeration method
    queryctrl = v4l2.v4l2_queryctrl(v4l2.V4L2_CID_BASE)

    while queryctrl.id < v4l2.V4L2_CID_LASTP1:
        try:
            ioctl(fd, v4l2.VIDIOC_QUERYCTRL, queryctrl)
        except IOError, e:
            # this predefined control is not supported by this device
            assert e.errno == errno.EINVAL
            queryctrl.id += 1
            continue
        yield queryctrl
        queryctrl = v4l2.v4l2_queryctrl(queryctrl.id + 1)

    queryctrl.id = v4l2.V4L2_CID_PRIVATE_BASE
    while True:
        try:
            ioctl(fd, v4l2.VIDIOC_QUERYCTRL, queryctrl)
        except IOError, e:
            # no more custom controls available on this device
            assert e.errno == errno.EINVAL
            break
        yield queryctrl
        queryctrl = v4l2.v4l2_queryctrl(queryctrl.id + 1)
    
    
def get_device_controls_by_class(fd, control_class):
    # enumeration by control class
    queryctrl = v4l2.v4l2_queryctrl(
        control_class | v4l2.V4L2_CTRL_FLAG_NEXT_CTRL)

    while True:
        try:
            ioctl(fd, v4l2.VIDIOC_QUERYCTRL, queryctrl)
        except IOError, e:
            assert e.errno == errno.EINVAL
            break
        if (v4l2.V4L2_CTRL_ID2CLASS(queryctrl.id) != control_class):
            break
        yield queryctrl
        queryctrl = v4l2.v4l2_queryctrl(
            queryctrl.id | v4l2.V4L2_CTRL_FLAG_NEXT_CTRL)
    

def get_device_controls_menu(fd, queryctrl):
    querymenu = v4l2.v4l2_querymenu(queryctrl.id, queryctrl.minimum)
    while querymenu.index <= queryctrl.maximum:
        ioctl(fd, v4l2.VIDIOC_QUERYMENU, querymenu)
        yield querymenu
        querymenu.index += 1


#
# tests
#

def test_VIDIOC_QUERYCAP(fd):
    cap = v4l2.v4l2_capability()

    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)

    assert 0 < len(cap.driver)
    assert valid_string(cap.card)
    # bus_info is allowed to be an empty string
    assert valid_string(cap.bus_info)
    assert valid_capabilities(cap.capabilities)
    assert cap.reserved[0] == 0
    assert cap.reserved[1] == 0
    assert cap.reserved[2] == 0
    assert cap.reserved[3] == 0


def test_VIDIOC_G_INPUT(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)

    if not cap.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE:
        # test does not apply for this device
        return

    index = v4l2.c_int()
    ioctl(fd, v4l2.VIDIOC_G_INPUT, index)


def test_VIDIOC_S_INPUT(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)

    if not cap.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE:
        # test does not apply for this device
        return

    index = v4l2.c_int(0)
    try:
        ioctl(fd, v4l2.VIDIOC_S_INPUT, index)
    except IOError, e:
        assert e.errno == errno.EBUSY
        return

    index.value = 1 << 31
    try:
        ioctl(fd, v4l2.VIDIOC_S_INPUT, index)
    except IOError, e:
        assert e.errno == errno.EINVAL


def test_VIDIOC_G_OUTPUT(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)

    if not cap.capabilities & v4l2.V4L2_CAP_VIDEO_OUTPUT:
        # test does not apply for this device
        return

    index = v4l2.c_int()
    ioctl(fd, v4l2.VIDIOC_G_OUTPUT, index)


def test_VIDIOC_S_OUTPUT(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)

    if not cap.capabilities & v4l2.V4L2_CAP_VIDEO_OUTPUT:
        # test does not apply for this device
        return

    index = v4l2.c_int(0)
    ioctl(fd, v4l2.VIDIOC_S_OUTPUT, index)

    index.value = 1 << 31
    try:
        ioctl(fd, v4l2.VIDIOC_S_OUTPUT, index)
    except IOError, e:
        assert e.errno == errno.EINVAL


def test_VIDIOC_ENUMINPUT(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)

    if not cap.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE:
        # test does not apply for this device
        return

    def assert_valid_input(fd, input_):
        # assert input_.index == index
        assert valid_string(input_.name)
        assert input_.type & (
            v4l2.V4L2_INPUT_TYPE_CAMERA | v4l2.V4L2_INPUT_TYPE_TUNER)
        assert input_.audioset < 32
        assert valid_v4l2_std_id(input_.std)
        if input_.status:
            assert valid_input_status(status)
        assert input_.reserved[0] == 0
        assert input_.reserved[1] == 0
        assert input_.reserved[2] == 0
        assert input_.reserved[3] == 0

    foreach_device_input(fd, assert_valid_input)


def test_VIDIOC_ENUMOUTPUT(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)

    if not cap.capabilities & v4l2.V4L2_CAP_VIDEO_OUTPUT:
        # test does not apply for this device
        return

    def assert_valid_output(fd, output):
        # assert output.index == index
        assert valid_string(output.name)
        assert output.type & (
            v4l2.V4L2_OUTPUT_TYPE_MODULATOR | v4l2.V4L2_OUTPUT_TYPE_ANALOG |
            v4l2.V4L2_OUTPUT_TYPE_ANALOGVGAOVERLAY)
        assert output.audioset < 32
        # assert output.modulator ?
        assert valid_v4l2_std_id(output.std)
        assert output.reserved == 0

    foreach_device_output(fd, assert_valid_ouput)


def test_VIDIOC_ENUMSTD(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)

    def assert_valid_standard(index, std):
        # assert std.index == index
        assert valid_v4l2_std_id(std.id)
        assert valid_string(std.name)
        assert std.frameperiod.numerator != 0
        assert std.frameperiod.denominator != 0
        assert std.framelines != 0
        assert std.reserved[0] == 0
        assert std.reserved[1] == 0
        assert std.reserved[2] == 0
        assert std.reserved[3] == 0

    # The spec says that a different set of standards may be yielded
    # by different inputs and outputs, so we repeat the test for each
    # available input and output of the current device.

    def test_available_standards(fd, input_or_output):
        for index, std in enumerate(get_device_standards(fd)):
            assert_valid_standard(index, std)

    # test for input devices
    if cap.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE:
        foreach_device_input(fd, test_available_standards)

    # test for output devices
    if cap.capabilities & v4l2.V4L2_CAP_VIDEO_OUTPUT:
        foreach_device_output(fd, test_available_standards)


def test_VIDIOC_G_STD(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)

    def test_get_standard(fd, input_or_output):
        std_id = v4l2.v4l2_std_id()
        try:
            ioctl(fd, v4l2.VIDIOC_G_STD, std_id)
        except IOError, e:
            assert e.errno == errno.EINVAL
            # input/output may not support a standard
            return
        assert valid_v4l2_std_id(std_id.value)

    # test for input devices
    if cap.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE:
        foreach_device_input(fd, test_get_standard)

    # test for output devices
    if cap.capabilities & v4l2.V4L2_CAP_VIDEO_OUTPUT:
        foreach_device_output(fd, test_get_standard)


def test_VIDIOC_S_STD(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)

    def test_set_standard(fd, input_or_output):
        original_std_id = v4l2.v4l2_std_id()
        try:
            ioctl(fd, v4l2.VIDIOC_G_STD, original_std_id)
        except IOError, e:
            assert e.errno == errno.EINVAL
            return

        for std in get_device_standards(fd):
            std_id = v4l2.v4l2_std_id(std.id)
            ioctl(fd, v4l2.VIDIOC_S_STD, std_id)

        bad_std_id = v4l2.v4l2_std_id(1 << 31)
        try:
            ioctl(fd, v4l2.VIDIOC_S_STD, bad_std_id)
        except IOError, e:
            assert e.errno == errno.EINVAL

        ioctl(fd, v4l2.VIDIOC_S_STD, original_std_id)

    # test for input devices
    if cap.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE:
        foreach_device_input(fd, test_set_standard)

    # test for output devices
    if cap.capabilities & v4l2.V4L2_CAP_VIDEO_OUTPUT:
        foreach_device_output(fd, test_set_standard)


def test_VIDIOC_QUERYSTD(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)

    def test_query_standard(fd, input_):
        std_id = v4l2.v4l2_std_id()
        try:
            ioctl(fd, v4l2.VIDIOC_QUERYSTD, std_id)
        except IOError, e:
            # this ioctl might not be supported on this device
            assert e.errno == errno.EINVAL
            return
        assert valid_v4l2_std_id(std_id.value)

    # test for input devices
    if cap.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE:
        foreach_device_input(fd, test_query_standard)


# Although the spec doesn't indicate this, device control tests
# will be applied to all inputs and outputs available on device.

def test_VIDIOC_QUERYCTRL(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)

    def assert_valid_queryctrl(queryctrl):
        assert queryctrl.type & (
            v4l2.V4L2_CTRL_TYPE_INTEGER |
            v4l2.V4L2_CTRL_TYPE_BOOLEAN |
            v4l2.V4L2_CTRL_TYPE_MENU |
            v4l2.V4L2_CTRL_TYPE_BUTTON |
            v4l2.V4L2_CTRL_TYPE_INTEGER64 |
            v4l2.V4L2_CTRL_TYPE_CTRL_CLASS)
        assert valid_string(queryctrl.name)
        assert queryctrl.minimum < queryctrl.maximum
        assert queryctrl.step > 0
        if queryctrl.flags:
            assert queryctrl.flags & (
                v4l2.V4L2_CTRL_FLAG_DISABLED |
                v4l2.V4L2_CTRL_FLAG_GRABBED |
                v4l2.V4L2_CTRL_FLAG_READ_ONLY |
                v4l2.V4L2_CTRL_FLAG_UPDATE |
                v4l2.V4L2_CTRL_FLAG_INACTIVE |
                v4l2.V4L2_CTRL_FLAG_SLIDER)
        assert queryctrl.reserved[0] == 0
        assert queryctrl.reserved[1] == 0

    def test_controls(fd, input_or_output):
        # original enumeration method
        for queryctrl in get_device_controls(fd):
            assert_valid_queryctrl(queryctrl)

        # enumeration by control class
        for class_ in (
            v4l2.V4L2_CTRL_CLASS_USER,
            v4l2.V4L2_CTRL_CLASS_MPEG,
            v4l2.V4L2_CTRL_CLASS_CAMERA):
            for queryctrl in get_device_controls_by_class(fd, class_):
                assert_valid_queryctrl(queryctrl)

    # general test
    foreach_device_input(fd, test_controls)

    # test for each input devices
    if cap.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE:
        foreach_device_input(fd, test_controls)

    # test for each output devices
    if cap.capabilities & v4l2.V4L2_CAP_VIDEO_OUTPUT:
        foreach_device_output(fd, test_controls)


def test_VIDIOC_QUERYMENU(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)

    def test_query_menu(fd, input_or_output):
        for queryctrl in get_device_controls(fd):
            if queryctrl.type == v4l2.V4L2_CTRL_TYPE_MENU:
                for querymenu in get_device_controls_menu(fd, queryctrl):
                    assert valid_string(querymenu.name)
                    assert querymenu.reserved == 0
        
    # general test
    foreach_device_input(fd, test_query_menu)

    # test for each input devices
    if cap.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE:
        foreach_device_input(fd, test_query_menu)

    # test for each output devices
    if cap.capabilities & v4l2.V4L2_CAP_VIDEO_OUTPUT:
        foreach_device_output(fd, test_query_menu)


def test_VIDIOC_G_CTRL(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)
    
    def test_get_control(fd, input_or_output):
        for queryctrl in get_device_controls(fd):
            if queryctrl.flags & v4l2.V4L2_CTRL_FLAG_DISABLED:
                continue

            control = v4l2.v4l2_control(queryctrl.id)
            ioctl(fd, v4l2.VIDIOC_G_CTRL, control)

            assert control.value >= queryctrl.minimum
            assert control.value <= queryctrl.maximum

    # general test
    foreach_device_input(fd, test_get_control)

    # test for each input devices
    if cap.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE:
        foreach_device_input(fd, test_get_control)

    # test for each output devices
    if cap.capabilities & v4l2.V4L2_CAP_VIDEO_OUTPUT:
        foreach_device_output(fd, test_get_control)


def test_VIDIOC_S_CTRL(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)
    
    def test_set_control(fd, input_or_output):
        for queryctrl in get_device_controls(fd):
            if queryctrl.flags & v4l2.V4L2_CTRL_FLAG_DISABLED:
                continue

            original_control = v4l2.v4l2_control(queryctrl.id)
            ioctl(fd, v4l2.VIDIOC_G_CTRL, original_control)

            control = v4l2.v4l2_control(queryctrl.id, queryctrl.default)
            ioctl(fd, v4l2.VIDIOC_S_CTRL, control)
            control.value = queryctrl.minimum + queryctrl.step
            ioctl(fd, v4l2.VIDIOC_S_CTRL, control)

            control.value = queryctrl.minimum - queryctrl.step
            try:
                ioctl(fd, v4l2.VIDIOC_S_CTRL, control)
            except IOError, e:
                assert e.errno in (
                    errno.ERANGE, errno.EINVAL, errno.EIO)
            control.value = queryctrl.maximum + queryctrl.step
            try:
                ioctl(fd, v4l2.VIDIOC_S_CTRL, control)
            except IOError, e:
                assert e.errno in (
                    errno.ERANGE, errno.EINVAL, errno.EIO)
            if queryctrl.step > 1:
                control.value = queryctrl.default + queryctrl.step - 1
                try:
                    ioctl(fd, v4l2.VIDIOC_S_CTRL, control)
                except IOError, e:
                    assert e.errno == errno.ERANGE

            ioctl(fd, v4l2.VIDIOC_S_CTRL, original_control)

    # general test
    foreach_device_input(fd, test_set_control)

    # test for each input devices
    if cap.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE:
        foreach_device_input(fd, test_set_control)

    # test for each output devices
    if cap.capabilities & v4l2.V4L2_CAP_VIDEO_OUTPUT:
        foreach_device_output(fd, test_set_control)


def test_VIDIOC_G_EXT_CTRLS(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)

    for class_ in (
        v4l2.V4L2_CTRL_CLASS_USER,
        v4l2.V4L2_CTRL_CLASS_MPEG,
        v4l2.V4L2_CTRL_CLASS_CAMERA):

        def get_controls(fd, input_or_output):
            # first we get the controls through enumeration
            # note, currently not distinguishing by disabled flag.
            queryctrls = list(get_device_controls_by_class(fd, class_))

            control_array = (v4l2.v4l2_ext_control * len(queryctrls))()
            for index, queryctrl in enumerate(queryctrls):
                control_array[index].id = queryctrl.id
            ext_controls = v4l2.v4l2_ext_controls(class_, len(queryctrls))
            ext_controls.controls = control_array

            try:
                ioctl(fd, v4l2.VIDIOC_G_EXT_CTRLS, ext_controls)
            except IOError, e:
                assert e.errno == errno.EINVAL
                assert not queryctrls
                return

            assert ext_controls.error_idx == 0
            assert ext_controls.reserved[0] == 0
            assert ext_controls.reserved[1] == 0
            for index, control in enumerate(control_array):
                assert control.value >= queryctrls[index].minimum
                assert control.value <= queryctrls[index].maximum

        # general test
        foreach_device_input(fd, get_controls)

        # test for each input devices
        if cap.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE:
            foreach_device_input(fd, get_controls)

        # test for each output devices
        if cap.capabilities & v4l2.V4L2_CAP_VIDEO_OUTPUT:
            foreach_device_output(fd, get_controls)


def test_VIDIOC_S_EXT_CTRLS(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)

    for class_ in (
        v4l2.V4L2_CTRL_CLASS_USER,
        v4l2.V4L2_CTRL_CLASS_MPEG,
        v4l2.V4L2_CTRL_CLASS_CAMERA):

        def set_controls(fd, input_or_output):
            # first we get the controls through enumeration
            queryctrls = list(get_device_controls_by_class(fd, class_))

            control_array = (v4l2.v4l2_ext_control * len(queryctrls))()
            for index, queryctrl in enumerate(queryctrls):
                control_array[index].id = queryctrl.id
            ext_controls = v4l2.v4l2_ext_controls(class_, len(queryctrls))
            ext_controls.controls = control_array

            # we store the original values so we can set them back later
            try:
                ioctl(fd, v4l2.VIDIOC_G_EXT_CTRLS, ext_controls)
            except IOError, e:
                assert e.errno == errno.EINVAL
                assert not queryctrls
                return
            original_values = [(c.value, c.value64) for c in control_array]

            # set to minimum value
            for index, control in enumerate(control_array):
                control.value = queryctrls[index].minimum
                control.value64 = queryctrls[index].minimum
            ioctl(fd, v4l2.VIDIOC_S_EXT_CTRLS, ext_controls)

            # test invalid control
            if queryctrls:
                control_array[-1].value = 1 << 31
                control_array[-1].value64 = 1 << 32
                try:
                    ioctl(fd, v4l2.VIDIOC_S_EXT_CTRLS, ext_controls)
                except IOError, e:
                    # the driver may either prune the value or raise
                    # ERANGE if control value is out of bounds
                    assert e.errno == errno.ERANGE
                    assert ext_controls.error_idx == len(control_array) - 1

            # set back original values
            for index, control in enumerate(control_array):
                control.value = original_values[index][0]
                control.value64 = original_values[index][1]
            ioctl(fd, v4l2.VIDIOC_S_EXT_CTRLS, ext_controls)

        # general test
        foreach_device_input(fd, set_controls)

        # test for each input devices
        if cap.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE:
            foreach_device_input(fd, set_controls)

        # test for each output devices
        if cap.capabilities & v4l2.V4L2_CAP_VIDEO_OUTPUT:
            foreach_device_output(fd, set_controls)


def test_VIDIOC_TRY_EXT_CTRLS(fd):
    cap = v4l2.v4l2_capability()
    ioctl(fd, v4l2.VIDIOC_QUERYCAP, cap)

    for class_ in (
        v4l2.V4L2_CTRL_CLASS_USER,
        v4l2.V4L2_CTRL_CLASS_MPEG,
        v4l2.V4L2_CTRL_CLASS_CAMERA):

        def try_controls(fd, input_or_output):
            # first we get the controls through enumeration
            queryctrls = list(get_device_controls_by_class(fd, class_))

            # try sane values
            control_array = (v4l2.v4l2_ext_control * len(queryctrls))()
            for index, queryctrl in enumerate(queryctrls):
                control_array[index].id = queryctrl.id
                control_array[index].value = queryctrl.default
            ext_controls = v4l2.v4l2_ext_controls(class_, len(queryctrls))
            ext_controls.controls = control_array
            try:
                ioctl(fd, v4l2.VIDIOC_TRY_EXT_CTRLS, ext_controls)
            except IOError, e:
                # driver may raise EINVAL if the control array has a
                # length of zero
                assert e.errno == errno.EINVAL
                assert not queryctrls
                return

            # try invalid values
            for index, control in enumerate(control_array):
                control.value = queryctrls[index].maximum + 1
                control.value64 = queryctrls[index].maximum + 1
            try:
                ioctl(fd, v4l2.VIDIOC_TRY_EXT_CTRLS, ext_controls)
            except IOError, e:
                assert e.errno == errno.EINVAL
                assert ext_controls.error_idx != 0

        # general test
        foreach_device_input(fd, try_controls)

        # test for each input devices
        if cap.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE:
            foreach_device_input(fd, try_controls)

        # test for each output devices
        if cap.capabilities & v4l2.V4L2_CAP_VIDEO_OUTPUT:
            foreach_device_output(fd, try_controls)


#
# bootstrap
#

devices = None


def open_devices():
    import glob
    global devices
    if devices is None:
        devices = [
            open(device, 'rw')
            for device in glob.glob('/dev/video*')]
        assert devices, 'No video devices found.'


def pytest_generate_tests(metafunc):
    open_devices()
    for fd in devices:
        metafunc.addcall(funcargs=dict(fd=fd))


def run():
    import sys
    open_devices()

    tests = [
        obj for member, obj in globals().items()
        if member.startswith('test_')]
    tests.sort(key=lambda t: id(t))

    for testfunc in tests:
        for fd in devices:
            try:
                testfunc(fd)
            except:
                sys.stderr.write('fd = %r\n' % fd)
                raise


if __name__ == '__main__':
    run()

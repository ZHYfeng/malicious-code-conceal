#!/usr/bin/env python
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

"""

This file defines the interface that can be used to
   fetch and update system configuration information,
   as well as the data object returned by the

"""
import os

from gppylib.gplog import *
from gppylib.utils import checkNotNone
from gppylib.system.fileSystemInterface import GpFileSystemProvider

from tempfile import NamedTemporaryFile

logger = get_default_logger()

#
# An implementation of GpFileSystemProvider that passes operations through to the underlying
#  operating system
#
class GpFileSystemProviderUsingOs(GpFileSystemProvider) :
    def __init__(self):
        pass

    #
    # Shutdown the file system provider
    #
    def destroy(self):
        pass

    #
    # Create a temporary file.  Note that this uses python's NamedTemporaryFile which
    #    may not do what we want on windows (that is, on windows, the temporary file
    #    cannot be opened for reading while it is opened for this write
    #
    # returns self
    #
    def createNamedTemporaryFile( self ) :
        return NamedTemporaryFile('w', delete=True)

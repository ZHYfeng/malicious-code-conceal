# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'includes': [
    '../../native_client/build/untrusted.gypi',
  ],
  'targets': [
    {
      'target_name': 'protobuf_lite_nacl',
      'type': 'none',
      'variables': {
        'nacl_untrusted_build': 1,
        'nlib_target': 'libprotobuf_lite_nacl.a',
        'build_glibc': 0,
        'build_newlib': 0,
        'build_pnacl_newlib': 1,
      },
      'includes': [
        'protobuf_lite.gypi',
      ],
    },
  ]
}

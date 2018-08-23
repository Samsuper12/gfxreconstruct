#!/usr/bin/env python
#
# Copyright (c) 2018 LunarG, Inc.
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

import os
import sys
import subprocess

# Relative path from code generators to directory containing the Vulkan XML Registry.
registry_path = '../external/Vulkan-Headers/registry'

# Relative path to vulkan code generators for trace encode/decode.
generator_path = './vulkan_generators'

# File names to provide to the Vulkan XML Registry generator script.
generate_targets = [
    'generated_encode_pnext_struct.inc',
    'generated_struct_encoders.inc',
    'generated_struct_encoder_declarations.inc',
    'generated_api_call_encoders.inc',
    'generated_layer_func_table.inc',
    'generated_decoded_struct_types.inc',
    'generated_struct_decoders.inc',
    'generated_struct_decoder_declarations.inc',
    'generated_api_call_decoders.inc',
    'generated_api_call_decoder_declarations.inc',
    'generated_api_call_decode_cases.inc',
    'generated_decode_pnext_struct.inc',
    'generated_api_call_consumer_declarations.inc',
    'generated_api_call_consumer_override_declarations.inc',
    'generated_api_call_ascii_consumer_definitions.inc',
    'generated_api_call_replay_consumer_definitions.inc'
]

if __name__ == '__main__':
    current_dir = os.path.dirname(os.path.abspath(sys.argv[0]))
    generator_dir = os.path.normpath(os.path.join(current_dir, generator_path))
    registry_dir = os.path.normpath(os.path.join(current_dir, registry_path))

    sys.path.append(generator_dir)
    sys.path.append(registry_dir)

    env = os.environ
    env['PYTHONPATH'] = os.pathsep.join(sys.path)

    for target in generate_targets:
        print('\nGenerating', target)
        subprocess.call([sys.executable, os.path.join(generator_dir, 'gencode.py'), '-o', current_dir, '-configs', generator_dir, '-registry', os.path.join(registry_dir, 'vk.xml'), target], shell=False, env=env)
import sys, os

idx = 0
out = ['#include "combined_waveforms.h"']

names = []

for arg in sys.argv:
    if arg.endswith('.c'):
        with open(arg) as f:
            s = f.read()
        name = os.path.basename(arg).removesuffix('.c')
        s = s.replace('_length', f'_length_{name}')
        s = s.replace('_data', f'_data_{name}')
        s = s.replace('unsigned short', f'uint16_t')
        s = s.replace('unsigned', f'uint16_t')
        out.append(f'/////// Waveform {arg} ////////')
        names.append(name)
        out.append(s)
        idx += 1

out.append('const uint16_t *get_waveform(int id, uint16_t *length)')
out.append('{')
out.append('    switch (id)')
out.append('    {')
for name in names:
    out.append(f'        case WAVEFORM_ID_{name}:')
    out.append(f'            *length = _length_{name};')
    out.append(f'            return _data_{name};')
out.append('        default:')
out.append(f'            return 0;')
out.append('    }')
out.append('}')

with open('combined_waveforms.c', 'w') as f:
    f.write('\n'.join(out))

with open('combined_waveforms.h', 'w') as f:
    f.write('#pragma once\n')
    f.write('#include <stdint.h>\n')
    f.write('const uint16_t *get_waveform(int id, uint16_t *length);\n\n')
    f.write(f'#define NUM_WAVEFORMS {idx}\n\n')

    f.write('// Waveform indices\n\n')
    for i, name in enumerate(names):
        f.write(f'#define WAVEFORM_ID_{name} {i}\n')

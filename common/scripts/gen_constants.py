import json
import os
import sys

directory = sys.argv[-1]
const_set = os.environ.get('CONSTANT_SET')
if not const_set:
    const_set = 'default'

with open(os.path.join(directory, 'constant-sets.json')) as f:
    sets = json.load(f)

if const_set == '--list':
    for key in sets:
        if key != 'documentation':
            print(key)
    exit(0)

print(f'generating constants.h to {directory} for set named {const_set}')

data = sets[const_set]

source = ['// Generated using gen_constants.py', f'// From configuration "{const_set}"', '', '#pragma once', '']

for key in data:
    if doc := sets['documentation'].get(key):
        source.append('')
        source.append(f'/* {doc} */')
    source.append(f'#define {key} {data[key]}')

source.append('')

with open(os.path.join(directory, 'constants.h'), 'w') as f:
    f.write('\n'.join(source))


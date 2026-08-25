#!/usr/bin/python3
import json
import os
import sys

json_file = os.path.join(sys.argv[-1], 'project.json')
action = sys.argv[-2]

if not os.path.exists(json_file):
    exit(0)

with open(json_file) as f:
    data = json.load(f)

if action == 'variants':
    variants = data.get('variants', [])
    for key in variants:
        print(key)
if action == "is_experimental" and data.get('experimental', False):
    print('experimental')


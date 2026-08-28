import sys

readme = sys.argv[-1]

with open(readme) as f:
    lines = f.readlines()

print("""
<head>
<style>
* { font-family: sans-serif; }
.knob {
width:50px;
height:50px;
background-color:darkslategray;
border-radius:50%;
display: inline-block;
}
table, th, td { border: 1px solid; }
</style>
</head>
""")

knob_template = '<div class="knob"></div><div>TEXT</div>'



state = 'init'
param_state = ''

for line in lines:
	if line.startswith('- '):
		if state != 'params':
			print('<table>')
		state = 'params'
		line = line.removeprefix('- ')
		if line[0].isdigit() and line[1] == ':':
			print(f'<tr><th>User param {line[0]}</th><td>{line}')
			param_state = 'print'
		if line.upper().startswith('TIME'):
			print(f'<tr><th>{knob_template.replace("TEXT", "TIME")}</th><td>{line}')
			param_state = 'print'
		if line.upper().startswith('DEPTH'):
			print(f'<tr><th>{knob_template.replace("TEXT", "DEPTH")}</th><td>{line}')
			param_state = 'print'
		if line.upper().startswith('SHAPE'):
			print(f'<tr><th>{knob_template.replace("TEXT", "SHAPE")}</th><td>{line}')
			param_state = 'print'
		if line.upper().startswith('SHIFT + SHAPE'):
			print(f'<tr><th>{knob_template.replace("TEXT", "SHIFT + SHAPE")}</th><td>{line}')
			param_state = 'print'
	elif param_state == 'print':
		print(f'<p>{line}</p>')
		if line.strip() == '':
			param_state = ''
			print('</td></tr>')
	if state == 'init' and line.startswith('#'):
		state = 'header'
		print(f'<h1>{line.replace('#', '')}</h1>')
	elif state == 'header' and line.startswith('#'):
		state = 'params'
		print('<table>')
	elif state == 'header':
		print(f'<p>{line}</p>')

if state == 'params':
	print('</table>')

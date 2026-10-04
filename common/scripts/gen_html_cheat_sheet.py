import sys

readme = sys.argv[-1]

with open(readme) as f:
    lines = f.readlines()

print("""
<head>
<style>
/* Base Layout & Neutral Technical Theme */
body {
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  background-color: #fcfcfc;
  color: #1a1a1a;
  line-height: 1.5;
  margin: 40px;
  padding: 0;
}

/* Typography */
h1 {
  color: #0f172a;
  font-size: 2rem;
  font-weight: 800;
  letter-spacing: -0.5px;
  margin-bottom: 20px;
  border-bottom: 2px solid #0f172a;
  padding-bottom: 8px;
  text-transform: uppercase;
}

/* Global paragraph styling */
p {
  margin: 6px 0;
  color: #334155;
}

/* Gracefully hides or collapses the empty spacer paragraphs */
p:empty {
  display: none;
}

/* Fluid Table Module Layout */
table {
  width: 100%;
  border-collapse: collapse;
  margin-top: 32px;
  border: 2px solid #0f172a;
  background-color: #ffffff;
}

th, td {
  padding: 16px;
  text-align: left;
  vertical-align: middle;
  border-bottom: 1px solid #e2e8f0;
}

tr:last-child th,
tr:last-child td {
  border-bottom: none;
}

/* Technical Sidebar Headers */
th {
  background-color: #f8fafc;
  width: 160px; /* Constrained only the control column width */
  text-align: center;
  font-size: 0.75rem;
  font-weight: 700;
  letter-spacing: 0.5px;
  color: #475569;
  border-right: 2px solid #0f172a;
}

/* Specific structural scoping for paragraphs inside table cells */
td p {
  margin: 2px 0;
  color: #0f172a;
}

/* The Knobs (Industrial Dial Aesthetic via existing class) */
.knob {
  width: 42px;
  height: 42px;
  background: radial-gradient(circle at 35% 35%, #ffffff, #e2e8f0);
  border: 3px solid #0f172a;
  border-radius: 50%;
  display: block;
  margin: 0 auto 8px auto;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
  position: relative;
}

/* Knob Indicator Line */
.knob::after {
  content: '';
  position: absolute;
  top: 3px;
  left: 50%;
  width: 3px;
  height: 10px;
  background-color: #0f172a;
  transform: translateX(-50%);
  border-radius: 1px;
}

/* Print Optimization Styles */
@media print {
  body {
    background-color: #ffffff;
    color: #000000;
    margin: 0;
    padding: 0;
  }

  table {
    page-break-inside: avoid;
  }

  th {
    background-color: #f1f5f9 !important;
    -webkit-print-color-adjust: exact;
    print-color-adjust: exact;
  }

  .knob {
    background: #ffffff !important;
    box-shadow: none !important;
  }
}

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
		if line.upper().startswith('SHIFT + DEPTH'):
			print(f'<tr><th>{knob_template.replace("TEXT", "SHIFT + DEPTH")}</th><td>{line}')
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

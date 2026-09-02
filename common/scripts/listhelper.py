def to_code(lst: list, typename: str = 'uint8_t', name: str = 'my_list') -> str:
	code = f'static {typename} {name}[{len(lst)}] =\n{{\n'
	line = '    '
	for val in lst:
		line += f'{val}, '
		if len(line) > 70:
			code += line + '\n'
			line = '    '
	code += line + '\n};'
	return code

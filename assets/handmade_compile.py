import os
import subprocess

def filename(file):
	return file.split(".")[0]

def compile(input, output):
	print(f"glslc.exe {input} -o {output}")
	subprocess.call(["glslc.exe", f"{input}", "-o", f"{output}"])

files = [f for f in os.listdir('.') if os.path.isfile(f)]
for file in files:
	output_file = filename(file)
	if file.endswith('.vert'):
		compile(file, f"{output_file}_vert.spv")
	if file.endswith('.frag'):
		compile(file, f"{output_file}_frag.spv")




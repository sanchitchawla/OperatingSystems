import os.path

def calculate(file):
	#execfile( "Makefile")
	total = 0.0
	i = 0
	if (not os.path.exists(file)):
		return "No file found"
	with open(file) as f:
		for line in f:
			i+=1
			# print line.split("\n")
			total+=int(line.split("\n")[0])
	return total/i

print calculate('test.txt')